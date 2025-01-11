# Report Rate Limit Input Processor

This module is used to limit the rate of input event sync from Zephyr input subsystem for ZMK. It is made for pointing device on split peripheral, to reduce the bluetooth connection loading between the peripherals and the central.

## What it does

It reduces all reporting input value and sync flag to zero and sum up the x/y delta, if report rate is too high (<8ms). The summarized value will be added up on next report event.

## Installation

Include this modulr on your ZMK's west manifest in `config/west.yml`:

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
/* common split inputs node on central and peripheral(s) */
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

/* add rate limit processor on peripheral(s) overlay */
#include <input/processors/report_rate_limit.dtsi>
&zip_report_rate_limit {
	report-ms = <12>; // reduce the report rate to 12ms, default is 8ms
};
&trackball_split {
  device = <&trackball>;
  input-processors = <&zip_report_rate_limit>;
};
```
