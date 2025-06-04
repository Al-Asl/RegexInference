## Profiling

This directory contains the profiling data for **Paresy**, captured on a laptop equipped with an RTX 2060 GPU and 6 GB of VRAM.

To view the profile, download and install  [NVIDIA Nsight Compute](https://developer.nvidia.com/nsight-compute). Once installed, launch Nsight Compute and open the `Paresy.ncu-proj` file. The profiling results will be displayed in the left-hand panel.

From the benchmarks, I selected the following results:
| Name | Time | IC Size | All REs | Cost |
|----------|----------|----------| ----------| ----------|
| type1_exp199 | 1.051885 s | 98 | 239.8 m | 19 |
| type1_exp50 | 20.54476 s | 25 | 903.6 m | 28 |
| type1_exp1 | 0.563667 s | 23 | 62.5 m | 20 |

Each benchmark produces two files `.nsys-rep` file, which provides a general performance overview. And `.ncu-rep` file, which contains detailed profiling data.