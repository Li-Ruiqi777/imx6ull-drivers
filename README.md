# imx6ull-drivers
imx6ull-mini board's linux driver

To use code intelligence properly, add this in c_cpp_properties.json

```json
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**",
                "/home/lrq/linux/IMX6ULL/temp/include",
                "/home/lrq/linux/IMX6ULL/temp/arch/arm/include",
                "/home/lrq/linux/IMX6ULL/temp/arch/arm/include/generated/"
            ],
            "defines": [],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```
