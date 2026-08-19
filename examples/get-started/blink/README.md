# blink

ESP-IDF-shaped hello world kept only as GPIO bring-up. It is **not**
a tracked feature. The product example is `examples/claw/edge_agent`.

On Pico W / Pico 2 W the default `CONFIG_BLINK_GPIO` is `32`
(`PIDF_GPIO_WL_LED`), the CYW43439 onboard LED.

```bash
export PIDF_PATH=/path/to/pico-idf
cd examples/get-started/blink
$PIDF_PATH/tools/pidf.py set-target pico_w
$PIDF_PATH/tools/pidf.py build
$PIDF_PATH/tools/pidf.py flash
$PIDF_PATH/tools/pidf.py monitor
```
