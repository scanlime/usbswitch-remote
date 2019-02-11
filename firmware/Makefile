APP := my-firmware
PROG := stlinkv2
CHIP := stm8s003f3

$(APP).ihx: $(APP).c
	sdcc -mstm8 $<

flash: $(APP).ihx
	stm8flash -c $(PROG) -p $(CHIP) -w $<

revert: orig-firmware.bin
	stm8flash -c $(PROG) -p $(CHIP) -w $<

clean:
	rm -f $(APP).asm $(APP).lst $(APP).cdb $(APP).ihx $(APP).lk $(APP).rst $(APP).map $(APP).rel $(APP).rst $(APP).sym

.PHONY: flash revert clean

