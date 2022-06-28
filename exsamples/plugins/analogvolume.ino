/**************************************************************

    An example of volume control using an analog resistor.
    This file must be in the root directory of the sketch.

**************************************************************/
const uint8_t volume_pin = 34;

void ctrls_on_loop() {
  static uint32_t prevVolPinMillis;
  uint16_t volPinVal = map(analogRead(volume_pin), 0, 4095, 0, 254);
  if((abs(volPinVal-config.store.volume)>2) && (millis()-prevVolPinMillis>300)){    /* simple debounce */
    player.setVol(volPinVal, false);
    prevVolPinMillis=millis();
  }
}
