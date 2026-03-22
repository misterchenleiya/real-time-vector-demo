BUILD_DIR ?= build
CMAKE ?= cmake
MODE ?= gui
CONFIG ?= configs/demo.receiver.50010.json
MEDIA ?=

.PHONY: build run clean

build:
	$(CMAKE) -S . -B $(BUILD_DIR)
	$(CMAKE) --build $(BUILD_DIR)

run: build
ifeq ($(MODE),gui)
	nohup ./$(BUILD_DIR)/demo_gui --config configs/demo.receiver.50010.json >/tmp/demo.receiver.50010.gui.log 2>&1 &
	nohup ./$(BUILD_DIR)/demo_gui --config configs/demo.receiver.50011.json >/tmp/demo.receiver.50011.gui.log 2>&1 &
	nohup ./$(BUILD_DIR)/demo_gui --config configs/demo.sender.json >/tmp/demo.sender.gui.log 2>&1 &
else ifeq ($(MODE),receiver)
	./$(BUILD_DIR)/demo_cli --mode receiver --config $(CONFIG)
else ifeq ($(MODE),sender)
ifeq ($(strip $(MEDIA)),)
	$(error MEDIA is required when MODE=sender)
endif
	./$(BUILD_DIR)/demo_cli --mode sender --config $(CONFIG) --media $(MEDIA)
else
	$(error Unsupported MODE "$(MODE)". Use MODE=gui, MODE=receiver, or MODE=sender)
endif

clean:
	$(CMAKE) -E rm -rf $(BUILD_DIR)
