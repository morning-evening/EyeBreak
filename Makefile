# ============================================================
# EyeBreak - Makefile
#
# 用法：
#   make          — Release 编译（无日志，优化级别 O2）
#   make debug    — Debug 编译（启用日志输出到 log/）
#   make clean    — 清理编译产物
#
# 前提：
#   - Visual Studio 2022 已安装（vcvars64.bat）
#   - 或 MSVC 命令行环境已配置
# ============================================================

.PHONY: all debug clean

CC      = cl.exe
RC      = rc.exe
TARGET  = dist\EyeBreak.exe
SRC     = src\eye_break.c
RES_SRC = res\resources.rc
RES_OUT = build\resources.res

CFLAGS  = /MT /O2 /utf-8 /W3 /I include
LDFLAGS = /SUBSYSTEM:WINDOWS
LIBS    = user32.lib shell32.lib advapi32.lib ole32.lib wtsapi32.lib

all: $(TARGET)

$(TARGET): $(SRC) $(RES_OUT)
	if not exist dist mkdir dist
	$(CC) $(CFLAGS) /Fe:$(TARGET) /Fobuild\ $(SRC) $(RES_OUT) /link $(LDFLAGS) $(LIBS)
	@echo.
	@echo === Build OK: $(TARGET) ===
	@if exist $(TARGET) for %%A in ($(TARGET)) do @echo Size: %%~zA bytes

$(RES_OUT): $(RES_SRC) include\resources.h res\tray_icon.ico
	if not exist build mkdir build
	$(RC) /I include /Fo $(RES_OUT) $(RES_SRC)

debug:
	if not exist dist mkdir dist
	if not exist build mkdir build
	$(RC) /I include /Fo build\resources.res $(RES_SRC)
	$(CC) $(CFLAGS) /DEYEBREAK_DEBUG /Fe:dist\EyeBreak_debug.exe /Fobuild\ $(SRC) build\resources.res /link $(LDFLAGS) $(LIBS)
	@echo.
	@echo === Debug Build OK (logging enabled to log/) ===

clean:
	-del /q build\*.obj 2>nul
	-del /q build\*.res 2>nul
	-del /q dist\*.exe 2>nul
	-del /q log\*.log 2>nul
	@echo === Clean done ===
