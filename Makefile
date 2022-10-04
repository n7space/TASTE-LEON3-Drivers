include definitions.mk

all: sis_module uart

sis_module: 
	$(MAKE) -C $(SIS_MODULE_SRC_DIR) sis

uart:
	$(MAKE) -C $(SRC_DIR) uart

leon3_serial_ccsds:
	$(MAKE) -C $(SRC_DIR) leon3_serial_ccsds

clean:
	$(MAKE) -C $(SIS_MODULE_SRC_DIR) clean
	$(MAKE) -C $(SRC_DIR) clean
	rm -rf $(BUILD_DIR)

.PHONY: clean

.DEFAULT_GOAL := all
