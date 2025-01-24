# Report Rate Limit Input Processor

This module is used to limit the rate of input reports that raised from pointing device on split peripheral, to reduce the bluetooth connection loading between the peripherals and the central.

## What it does

It reduces all reporting input value and sync flag to zero and proxying x/y delta while the input event rate is too high for each input code (<8ms). The summarized value will be added up on next report event. While the pointing device report a continuoues movement on an axis (with same input code), the value would be accumulated and report later.

## Installation

Include this module on your ZMK's west manifest in `config/west.yml`:

```yaml
manifest:
  remotes:
    #...
    # START #####
    - name: badjeff
      url-base: https://github.com/badjeff
    # END #######
    #...
  projects:
    #...
    # START #####
    - name: zmk-input-processor-report-rate-limit
      remote: badjeff
      revision: main
    # END #######
    #...
```

Roughly, `overlay` of the split-peripheral trackball should look like below.

```
/* Typical common split inputs node on central and peripheral(s) */
/{
  split_inputs {
    #address-cells = <1>;
    #size-cells = <0>;

    trackball_split: trackball@0 {
      compatible = "zmk,input-split";
      reg = <0>;
    };

  };
};

/* Add the rate limit processor on peripheral(s) overlay */
#include <input/processors/report_rate_limit.dtsi>

&trackball_split {
  device = <&trackball>;
  input-processors = <&zip_report_rate_limit 16>; // limit to 16ms, default is 16ms
};
```
