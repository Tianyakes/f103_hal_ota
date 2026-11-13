#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""STM32 Bootloader OTA 串口烧录上位机。

协议来源：
    Bootloader/Protocol/ota_protocol.c
    Bootloader/core/bl_conf.h
    帧头定义.drawio

一帧结构（多字节字段均为大端）：
    +-----------+-----+-------+-------+-------+----------+---------+
    | 帧头 0xA5A5| CMD |  len  | index | total |   data   |  CRC16  |
    |    2B     | 1B  |  2B   |  1B   |  1B   |  len B   |   2B    |
    +-----------+-----+-------+-------+-------+----------+---------+
    CRC16-XMODEM：poly=0x1021, init=0x0000, 不反射，
    覆盖“帧头..data”共 len+7 字节。
    应答为裸 ASCII："ACK" / "NACK"（无帧封装）。

流程：
    START --ACK--> DATA(index 1..n, 每包等待 ACK) --ACK--> FINISH --ACK--> 完成
    设备只有在 PA0 为低电平时才进入升级模式并接受帧，因此上位机循环发送
    START 进行握手，直到收到 ACK。

依赖：
    pip install pyserial
"""

import queue
import struct
import sys
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError:  # 在 main() 里给出友好提示
    serial = None
    list_ports = None

try:
    import ctypes

    ctypes.windll.shcore.SetProcessDpiAwareness(1)
except Exception:
    pass


# ----------------------------------------------------------------------------
# 协议常量（与 bl_conf.h 保持一致）
# ----------------------------------------------------------------------------
FRAME_HEAD = 0xA5A5
MAX_DATA_LEN = 512

CMD_START = 0x01
CMD_DATA = 0x02
CMD_FINISH = 0x03

ACK_TOKEN = b"ACK"
NACK_TOKEN = b"NACK"

APP_AREA_SIZE = 24 * 1024        # BOOTL_APPLICATION_SIZE
DOWNLOAD_AREA_SIZE = 23 * 1024   # BOOTL_DOWNLOAD_SIZE（仅用于界面提示）

HANDSHAKE_ATTEMPTS = 120         # 每次尝试发送 START 后等待 0.5s
PACKET_RETRIES = 4
FINISH_RETRIES = 2

APP_TITLE = "STM32 Bootloader OTA 烧录上位机"


class FlasherError(Exception):
    """协议/串口错误，携带可直接展示给用户的中文信息。"""


class FlashCancelled(Exception):
    """用户点击了停止。"""


# ----------------------------------------------------------------------------
# CRC16-XMODEM
# ----------------------------------------------------------------------------
def _build_crc_table():
    table = []
    for idx in range(256):
        crc = idx << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
        table.append(crc)
    return tuple(table)


_CRC_TABLE = _build_crc_table()


def crc16_xmodem(data):
    crc = 0
    for byte in data:
        crc = ((crc << 8) & 0xFFFF) ^ _CRC_TABLE[((crc >> 8) ^ byte) & 0xFF]
    return crc


def build_frame(cmd, index=0, total=0, payload=b""):
    """按协议拼帧：7 字节头 + 数据 + 2 字节 CRC。"""
    payload = bytes(payload)
    if len(payload) > MAX_DATA_LEN:
        raise ValueError("单帧数据不能超过 %d 字节" % MAX_DATA_LEN)
    body = struct.pack(
        ">HBHBB", FRAME_HEAD, cmd, len(payload), index & 0xFF, total & 0xFF
    ) + payload
    return body + struct.pack(">H", crc16_xmodem(body))


# ----------------------------------------------------------------------------
# 烧录逻辑（与界面无关，方便单独复用）
# ----------------------------------------------------------------------------
class OtaFlasher:
    def __init__(
        self,
        port,
        baud,
        packet_size=MAX_DATA_LEN,
        ack_timeout=5.0,
        packet_retries=PACKET_RETRIES,
        handshake_attempts=HANDSHAKE_ATTEMPTS,
        on_log=None,
        on_progress=None,
        on_state=None,
    ):
        self.port = port
        self.baud = baud
        self.packet_size = max(1, min(packet_size, MAX_DATA_LEN))
        self.ack_timeout = ack_timeout
        self.packet_retries = packet_retries
        self.handshake_attempts = handshake_attempts

        self._on_log = on_log or (lambda msg, level="info": None)
        self._on_progress = on_progress or (lambda sent, total: None)
        self._on_state = on_state or (lambda text, level="run": None)

        self._ser = None
        self._stop = threading.Event()

    # -- 外部控制 -----------------------------------------------------------
    def stop(self):
        self._stop.set()

    @property
    def stopped(self):
        return self._stop.is_set()

    def _log(self, msg, level="info"):
        self._on_log(msg, level)

    def _state(self, text, level="run"):
        self._on_state(text, level)

    # -- 串口底层 -----------------------------------------------------------
    def _open(self):
        try:
            self._ser = serial.Serial(
                port=self.port,
                baudrate=self.baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.05,
                write_timeout=2.0,
                rtscts=False,
                dsrdtr=False,
            )
        except (serial.SerialException, OSError) as exc:
            raise FlasherError(
                "无法打开串口 %s：%s（端口被占用或已拔出？）" % (self.port, exc)
            )
        time.sleep(0.1)  # 给 USB 转串口芯片一点稳定时间
        self._flush_input()

    def _close(self):
        if self._ser is not None:
            try:
                self._ser.close()
            except Exception:
                pass
            self._ser = None

    def _flush_input(self):
        try:
            self._ser.reset_input_buffer()
        except (serial.SerialException, OSError) as exc:
            raise FlasherError("串口连接已断开：%s" % exc)

    def _write(self, data):
        if self._stop.is_set():
            raise FlashCancelled()
        try:
            self._ser.write(data)
            self._ser.flush()
        except serial.SerialTimeoutException:
            raise FlasherError("串口写入超时，请检查波特率或连接")
        except (serial.SerialException, OSError) as exc:
            raise FlasherError("串口写入失败：%s" % exc)

    def _read_token(self, timeout):
        """在 timeout 内读取应答，返回 "ACK" / "NACK" / None。

        设备端只回裸 ASCII，这里按字节流扫描，容忍其它日志字符。
        先判断 NACK：它的尾部同样包含 "ACK"。
        """
        deadline = time.monotonic() + timeout
        buf = bytearray()
        while True:
            if self._stop.is_set():
                raise FlashCancelled()
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return None
            try:
                chunk = self._ser.read(64)
            except (serial.SerialException, OSError) as exc:
                raise FlasherError("串口读取失败：%s" % exc)
            if not chunk:
                continue
            buf.extend(chunk)

            i = 0
            while i < len(buf):
                if buf.startswith(NACK_TOKEN, i):
                    return "NACK"
                if buf.startswith(ACK_TOKEN, i):
                    return "ACK"
                i += 1
            # 没找到完整应答：只保留可能被截断的尾部，丢弃其它噪声
            if len(buf) > 8:
                del buf[:-8]

    def _transact(self, frame, timeout, retries, what):
        """发送一帧并等待 ACK，超时/NACK 自动重发。"""
        for attempt in range(1, retries + 1):
            self._flush_input()
            self._write(frame)
            token = self._read_token(timeout)
            if token == "ACK":
                return
            if token == "NACK":
                self._log("%s：收到 NACK（第 %d/%d 次）" % (what, attempt, retries), "warn")
            else:
                self._log("%s：应答超时（第 %d/%d 次）" % (what, attempt, retries), "warn")
        raise FlasherError("%s：重试 %d 次仍未收到 ACK，烧录中止" % (what, retries))

    # -- 协议阶段 -----------------------------------------------------------
    def _handshake(self):
        frame = build_frame(CMD_START)
        self._state("等待 Bootloader 握手…", "run")
        self._log("发送 START，等待设备应答（请确认 PA0 已拉低进入升级模式）")
        for attempt in range(1, self.handshake_attempts + 1):
            if self._stop.is_set():
                raise FlashCancelled()
            self._flush_input()
            self._write(frame)
            token = self._read_token(0.5)
            if token == "ACK":
                self._log("握手成功，Bootloader 已进入升级模式", "ok")
                time.sleep(0.05)
                self._flush_input()  # 丢弃重复 START 产生的多余 ACK
                return
            if token == "NACK":
                self._log("START 被 NACK，设备可能尚未进入升级模式", "warn")
            if attempt % 20 == 0:
                self._log("仍在等待握手…（已尝试 %d 次）" % attempt, "dim")
        raise FlasherError("握手失败：Bootloader 未应答，请检查 PA0、串口和波特率")

    def _flash_data(self, firmware):
        total = len(firmware)
        packet_count = (total + self.packet_size - 1) // self.packet_size
        if packet_count > 255:
            # index/total 只有 1 字节，设备并不使用这两个字段，按 uint8 回绕即可
            self._log(
                "提示：包数 %d 超过 255，index/total 字段会回绕（Bootloader 不使用）"
                % packet_count,
                "dim",
            )

        self._state("数据传输中…", "run")
        sent = 0
        log_stride = max(1, packet_count // 20)
        for i in range(packet_count):
            if self._stop.is_set():
                raise FlashCancelled()
            chunk = firmware[i * self.packet_size:(i + 1) * self.packet_size]
            frame = build_frame(CMD_DATA, index=i + 1, total=packet_count, payload=chunk)
            self._transact(
                frame,
                self.ack_timeout,
                self.packet_retries,
                "数据包 %d/%d" % (i + 1, packet_count),
            )
            sent += len(chunk)
            self._on_progress(sent, total)
            if sent == total or (i + 1) % log_stride == 0:
                self._log(
                    "进度 %d/%d 字节（%d/%d 包）"
                    % (sent, total, i + 1, packet_count),
                    "dim",
                )

    def _finish(self):
        self._state("等待设备搬运固件…", "run")
        timeout = max(self.ack_timeout * 3.0, 10.0)
        self._log("发送 FINISH，等待设备完成搬运")
        self._transact(build_frame(CMD_FINISH), timeout, FINISH_RETRIES, "FINISH")

    # -- 入口 ---------------------------------------------------------------
    def run(self, firmware):
        try:
            self._open()
            self._log("已打开 %s @ %d 8N1" % (self.port, self.baud))
            self._handshake()
            self._flash_data(firmware)
            self._finish()
            self._on_progress(len(firmware), len(firmware))
            self._state("烧录完成", "ok")
            self._log("烧录流程完成，FINISH 已确认", "ok")
        finally:
            self._close()


# ----------------------------------------------------------------------------
# Tkinter 界面
# ----------------------------------------------------------------------------
class App(tk.Tk):
    LEVEL_TAGS = ("info", "ok", "warn", "error", "dim")

    def __init__(self):
        super().__init__()
        self.title(APP_TITLE)
        self.geometry("900x640")
        self.minsize(820, 580)

        self._queue = queue.Queue()
        self._worker = None
        self._flasher = None
        self._firmware = None
        self._firmware_path = ""
        self._firmware_size = 0
        self._sent_bytes = 0
        self._job_start = 0.0
        self._running = False
        self._port_map = {}

        self._build_ui()
        self._refresh_ports()
        self._update_file_info()
        self.after(50, self._poll_queue)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    # -- 界面搭建 -----------------------------------------------------------
    def _build_ui(self):
        self.columnconfigure(0, weight=1)
        self.rowconfigure(4, weight=1)

        pad = {"padx": 6, "pady": 4}

        # 串口设置
        serial_box = ttk.LabelFrame(self, text="串口设置")
        serial_box.grid(row=0, column=0, sticky="ew", padx=10, pady=(10, 4))
        serial_box.columnconfigure(1, weight=1)
        serial_box.columnconfigure(4, weight=1)

        ttk.Label(serial_box, text="串口").grid(row=0, column=0, sticky="w", **pad)
        self.port_var = tk.StringVar()
        self.port_cb = ttk.Combobox(
            serial_box, textvariable=self.port_var, state="readonly"
        )
        self.port_cb.grid(row=0, column=1, columnspan=2, sticky="ew", **pad)
        self.refresh_btn = ttk.Button(
            serial_box, text="刷新", width=6, command=self._refresh_ports
        )
        self.refresh_btn.grid(row=0, column=3, **pad)

        ttk.Label(serial_box, text="波特率").grid(row=0, column=4, sticky="e", **pad)
        self.baud_var = tk.StringVar(value="115200")
        self.baud_cb = ttk.Combobox(
            serial_box,
            textvariable=self.baud_var,
            width=10,
            values=["9600", "19200", "38400", "57600", "115200",
                    "230400", "460800", "921600"],
        )
        self.baud_cb.grid(row=0, column=5, sticky="w", **pad)

        ttk.Label(serial_box, text="包大小").grid(row=1, column=0, sticky="w", **pad)
        self.packet_var = tk.StringVar(value=str(MAX_DATA_LEN))
        self.packet_cb = ttk.Combobox(
            serial_box,
            textvariable=self.packet_var,
            state="readonly",
            width=8,
            values=["512", "256", "128", "64"],
        )
        self.packet_cb.grid(row=1, column=1, sticky="w", **pad)
        self.packet_cb.bind("<<ComboboxSelected>>", self._on_packet_changed)

        ttk.Label(serial_box, text="应答超时").grid(row=1, column=4, sticky="e", **pad)
        timeout_frame = ttk.Frame(serial_box)
        timeout_frame.grid(row=1, column=5, sticky="w", **pad)
        self.timeout_var = tk.StringVar(value="5000")
        self.timeout_spin = ttk.Spinbox(
            timeout_frame, textvariable=self.timeout_var,
            from_=500, to=30000, increment=500, width=8,
        )
        self.timeout_spin.pack(side="left")
        ttk.Label(timeout_frame, text=" ms").pack(side="left", padx=(2, 0))

        # 固件
        file_box = ttk.LabelFrame(self, text="固件")
        file_box.grid(row=1, column=0, sticky="ew", padx=10, pady=4)
        file_box.columnconfigure(1, weight=1)

        self.browse_btn = ttk.Button(
            file_box, text="选择固件…", command=self._browse_firmware
        )
        self.browse_btn.grid(row=0, column=0, **pad)

        self.file_var = tk.StringVar(value="（未选择）")
        file_entry = ttk.Entry(
            file_box, textvariable=self.file_var, state="readonly"
        )
        file_entry.grid(row=0, column=1, sticky="ew", **pad)

        self.file_info_var = tk.StringVar(value="")
        ttk.Label(
            file_box, textvariable=self.file_info_var, foreground="#57606a"
        ).grid(row=1, column=0, columnspan=2, sticky="w", padx=6)

        ttk.Label(
            file_box,
            text="仅支持原始 .bin；固件写入应用程序区（起始 0x08004000，最大 24KB）。"
                 "开始前请先把 PA0 拉低。",
            foreground="#9a6700",
        ).grid(row=2, column=0, columnspan=2, sticky="w", padx=6, pady=(0, 4))

        # 操作
        action = ttk.Frame(self)
        action.grid(row=2, column=0, sticky="ew", padx=10, pady=4)

        self.start_btn = ttk.Button(
            action, text="开始烧录", command=self._on_start, width=14
        )
        self.start_btn.pack(side="left", padx=(0, 6))
        self.stop_btn = ttk.Button(
            action, text="停止", command=self._on_stop, width=10, state="disabled"
        )
        self.stop_btn.pack(side="left")

        self.status_var = tk.StringVar(value="就绪")
        self.status_label = tk.Label(
            action, textvariable=self.status_var, font=("Microsoft YaHei UI", 10, "bold")
        )
        self.status_label.pack(side="right", padx=6)
        ttk.Label(action, text="状态：").pack(side="right")

        # 进度
        progress_box = ttk.LabelFrame(self, text="传输进度")
        progress_box.grid(row=3, column=0, sticky="ew", padx=10, pady=4)
        progress_box.columnconfigure(0, weight=1)

        self.progress_var = tk.DoubleVar(value=0.0)
        self.progress_bar = ttk.Progressbar(
            progress_box, variable=self.progress_var, maximum=100.0
        )
        self.progress_bar.grid(row=0, column=0, sticky="ew", padx=8, pady=(8, 2))

        self.percent_var = tk.StringVar(value="0.0%")
        ttk.Label(
            progress_box, textvariable=self.percent_var, width=8, anchor="e"
        ).grid(row=0, column=1, padx=(0, 8))

        detail = ttk.Frame(progress_box)
        detail.grid(row=1, column=0, columnspan=2, sticky="ew", padx=8, pady=(0, 6))
        self.sent_var = tk.StringVar(value="已发送: 0 B")
        self.speed_var = tk.StringVar(value="速度: --")
        self.elapsed_var = tk.StringVar(value="用时: 0.0 s")
        for idx, var in enumerate((self.sent_var, self.speed_var, self.elapsed_var)):
            ttk.Label(detail, textvariable=var).grid(
                row=0, column=idx, sticky="w", padx=(0, 24)
            )

        # 日志
        log_box = ttk.LabelFrame(self, text="日志")
        log_box.grid(row=4, column=0, sticky="nsew", padx=10, pady=(4, 10))
        log_box.rowconfigure(1, weight=1)
        log_box.columnconfigure(0, weight=1)

        log_header = ttk.Frame(log_box)
        log_header.grid(row=0, column=0, columnspan=2, sticky="ew", padx=6, pady=(2, 0))
        ttk.Button(
            log_header, text="清空日志", width=10, command=self._clear_log
        ).pack(side="right")

        self.log_text = tk.Text(
            log_box, height=12, wrap="none", state="disabled",
            font=("Consolas", 9), background="#fbfbfb",
        )
        self.log_text.grid(row=1, column=0, sticky="nsew", padx=(6, 0), pady=(2, 6))
        scrollbar = ttk.Scrollbar(log_box, orient="vertical", command=self.log_text.yview)
        scrollbar.grid(row=1, column=1, sticky="ns", padx=(0, 6), pady=(2, 6))
        self.log_text.configure(yscrollcommand=scrollbar.set)
        self.log_text.tag_configure("ok", foreground="#1a7f37")
        self.log_text.tag_configure("warn", foreground="#9a6700")
        self.log_text.tag_configure("error", foreground="#cf222e")
        self.log_text.tag_configure("dim", foreground="#8c959f")

        self._stateful = [
            (self.port_cb, "readonly"),
            (self.refresh_btn, "normal"),
            (self.baud_cb, "normal"),
            (self.packet_cb, "readonly"),
            (self.timeout_spin, "normal"),
            (self.browse_btn, "normal"),
        ]

        self._append_log("就绪。选择串口和 .bin 固件后点击“开始烧录”。", "dim")

    # -- 串口枚举 -----------------------------------------------------------
    def _refresh_ports(self):
        ports = list(list_ports.comports())
        self._port_map = {}
        for p in ports:
            desc = (p.description or "").strip()
            key = "%s  (%s)" % (p.device, desc) if desc else p.device
            self._port_map[key] = p.device
        values = list(self._port_map.keys())
        self.port_cb.configure(values=values)
        if values and self.port_var.get() not in values:
            self.port_var.set(values[0])
        if not values:
            self.port_var.set("")
            self._append_log("未检测到串口，请连接设备后点击“刷新”。", "warn")

    # -- 固件选择 -----------------------------------------------------------
    def _browse_firmware(self):
        path = filedialog.askopenfilename(
            title="选择固件",
            filetypes=[("BIN 固件", "*.bin"), ("所有文件", "*.*")],
        )
        if not path:
            return
        try:
            with open(path, "rb") as fp:
                data = fp.read()
        except OSError as exc:
            messagebox.showerror("读取失败", "无法读取固件：\n%s" % exc)
            return
        if not data:
            messagebox.showerror("固件为空", "所选文件内容为空。")
            return
        self._firmware_path = path
        self._firmware = data
        self._firmware_size = len(data)
        self.file_var.set(path)
        self._update_file_info()
        self._append_log("已选择固件：%s（%s 字节）" % (path, format(len(data), ",")))

    def _on_packet_changed(self, _event=None):
        self._update_file_info()

    def _update_file_info(self):
        if not self._firmware:
            self.file_info_var.set("")
            return
        try:
            packet = int(self.packet_var.get())
        except ValueError:
            packet = MAX_DATA_LEN
        count = (self._firmware_size + packet - 1) // packet if self._firmware_size else 0
        self.file_info_var.set(
            "%s 字节  |  %d 包 × %d B" % (format(self._firmware_size, ","), count, packet)
        )

    # -- 日志与状态 ---------------------------------------------------------
    def _append_log(self, msg, level="info"):
        timestamp = time.strftime("%H:%M:%S")
        self.log_text.configure(state="normal")
        self.log_text.insert(
            "end", "[%s] %s\n" % (timestamp, msg), level if level in self.LEVEL_TAGS else "info"
        )
        self.log_text.configure(state="disabled")
        self.log_text.see("end")

    def _clear_log(self):
        self.log_text.configure(state="normal")
        self.log_text.delete("1.0", "end")
        self.log_text.configure(state="disabled")

    def _set_status(self, text, level="idle"):
        colors = {
            "idle": "#57606a",
            "run": "#0969da",
            "ok": "#1a7f37",
            "warn": "#9a6700",
            "err": "#cf222e",
        }
        self.status_var.set(text)
        self.status_label.configure(foreground=colors.get(level, "#57606a"))

    def _set_controls_enabled(self, enabled):
        for widget, state in self._stateful:
            widget.configure(state=state if enabled else "disabled")
        self.start_btn.configure(state="normal" if enabled else "disabled")
        self.stop_btn.configure(state="disabled" if enabled else "normal")

    # -- 线程 -> 界面消息 ---------------------------------------------------
    def _queue_log(self, msg, level="info"):
        self._queue.put(("log", msg, level))

    def _queue_progress(self, sent, total):
        self._queue.put(("progress", sent, total))

    def _queue_state(self, text, level="run"):
        self._queue.put(("status", text, level))

    def _poll_queue(self):
        try:
            while True:
                item = self._queue.get_nowait()
                kind = item[0]
                if kind == "log":
                    self._append_log(item[1], item[2])
                elif kind == "progress":
                    self._update_progress(item[1], item[2])
                elif kind == "status":
                    self._set_status(item[1], item[2])
                elif kind == "done":
                    self._on_done(item[1], item[2])
        except queue.Empty:
            pass
        self.after(50, self._poll_queue)

    def _update_progress(self, sent, total):
        self._sent_bytes = sent
        percent = sent * 100.0 / total if total else 0.0
        self.progress_var.set(percent)
        self.percent_var.set("%.1f%%" % percent)
        self.sent_var.set("已发送: %s / %s B" % (format(sent, ","), format(total, ",")))
        elapsed = max(1e-6, time.monotonic() - self._job_start)
        self.speed_var.set("速度: %.1f KB/s" % (sent / elapsed / 1024.0))
        self.elapsed_var.set("用时: %.1f s" % elapsed)

    def _tick(self):
        if not self._running:
            return
        elapsed = time.monotonic() - self._job_start
        self.elapsed_var.set("用时: %.1f s" % elapsed)
        self.after(200, self._tick)

    # -- 开始/停止/结束 -----------------------------------------------------
    def _on_start(self):
        if self._worker is not None:
            return

        port = self._port_map.get(self.port_var.get())
        if not port:
            messagebox.showerror("未选择串口", "请先选择串口，必要时点击“刷新”。")
            return
        if not self._firmware:
            messagebox.showerror("未选择固件", "请先选择一个 .bin 固件文件。")
            return
        if self._firmware_size > APP_AREA_SIZE:
            messagebox.showerror(
                "固件过大",
                "固件 %s 字节，超过应用程序区大小 %s 字节，无法烧录。"
                % (format(self._firmware_size, ","), format(APP_AREA_SIZE, ",")),
            )
            return
        try:
            baud = int(self.baud_var.get())
            packet = int(self.packet_var.get())
            ack_ms = int(self.timeout_var.get())
        except ValueError:
            messagebox.showerror("参数错误", "波特率、包大小、应答超时必须是整数。")
            return
        if baud <= 0:
            messagebox.showerror("参数错误", "波特率必须大于 0。")
            return
        if not 1 <= packet <= MAX_DATA_LEN:
            messagebox.showerror(
                "参数错误", "包大小必须在 1~%d 字节之间。" % MAX_DATA_LEN
            )
            return
        if not 200 <= ack_ms <= 60000:
            messagebox.showerror("参数错误", "应答超时必须在 200~60000 ms 之间。")
            return

        self._job_start = time.monotonic()
        self._sent_bytes = 0
        self.progress_var.set(0.0)
        self.percent_var.set("0.0%")
        self.sent_var.set(
            "已发送: 0 / %s B" % format(self._firmware_size, ",")
        )
        self.speed_var.set("速度: --")
        self.elapsed_var.set("用时: 0.0 s")

        flasher = OtaFlasher(
            port=port,
            baud=baud,
            packet_size=packet,
            ack_timeout=ack_ms / 1000.0,
            on_log=self._queue_log,
            on_progress=self._queue_progress,
            on_state=self._queue_state,
        )
        self._flasher = flasher
        self._running = True
        self._set_controls_enabled(False)
        self._set_status("启动中…", "run")
        self._append_log(
            "开始烧录：%s，%s 字节，%d B/包"
            % (self._firmware_path, format(self._firmware_size, ","), packet)
        )
        if self._firmware_size > DOWNLOAD_AREA_SIZE:
            self._append_log(
                "提示：固件大于下载区 23KB，设备会在传输中途分批搬运，属正常现象。",
                "dim",
            )

        self._worker = threading.Thread(
            target=self._worker_run, args=(flasher, self._firmware), daemon=True
        )
        self._worker.start()
        self._tick()

    def _worker_run(self, flasher, firmware):
        try:
            flasher.run(firmware)
            self._queue.put(("done", True, ""))
        except FlashCancelled:
            self._queue.put(("log", "烧录已由用户停止", "warn"))
            self._queue.put(("done", False, "已停止"))
        except FlasherError as exc:
            self._queue.put(("log", str(exc), "error"))
            self._queue.put(("done", False, "烧录失败"))
        except Exception as exc:  # 兜底，避免线程异常无声退出
            self._queue.put(("log", "未预期错误：%r" % exc, "error"))
            self._queue.put(("done", False, "烧录失败"))

    def _on_stop(self):
        if self._flasher is not None:
            self._append_log("正在停止…", "warn")
            self._set_status("停止中…", "warn")
            self._flasher.stop()
            self.stop_btn.configure(state="disabled")

    def _on_done(self, success, message):
        self._running = False
        self._worker = None
        self._flasher = None
        self._set_controls_enabled(True)

        if success:
            self.progress_var.set(100.0)
            self.percent_var.set("100.0%")
            self._set_status("烧录完成", "ok")
        elif message == "已停止":
            self._set_status("已停止", "warn")
        else:
            self._set_status("烧录失败", "err")

    def _on_close(self):
        if self._worker is not None and self._worker.is_alive():
            if not messagebox.askyesno("退出", "烧录仍在进行，确定停止并退出吗？"):
                return
            if self._flasher is not None:
                self._flasher.stop()
            self._worker.join(timeout=2.0)
        self.destroy()


def main():
    if serial is None:
        root = tk.Tk()
        root.withdraw()
        messagebox.showerror(
            "缺少依赖",
            "未安装 pyserial，请先运行：\n\npip install pyserial",
        )
        root.destroy()
        return 1
    app = App()
    app.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
