<div class="markdown-google-sans">
<h1><strong>Regex Inference On The GPU</strong></h1>
</div>

This repository contains the code and related resources for the **scaled** version of the Parallel Regular Expression Synthesiser (**PaRESy**) by *Mojtaba Valizadeh* and *Martin Berger*.  

For more details on their work, refer to their [paper](https://dl.acm.org/doi/10.1145/3591274).

## Introduction

This work aims to explore new approaches for scaling **PaRESy** by sacrificing `minimality` while preserving `precision`. The inference process takes as input a set of positive strings, a set of negative strings, and a cost function. It then produces a regular expression that accepts all positive strings, rejects all negative strings, and remains as close to minimal cost as possible.

In this version of the work, a simple grammar have been used for the REs:

```
R ::= Φ|ε|a|R?|R*|R.R|R+R|R&R
```
For minimality, a cost function is defined that assigns a positive integer to each constructor in the regular expression (RE). The total cost of an RE is the sum of its constructors’ costs. This approach helps prevent overfitting and avoids producing the trivial RE that is simply the union of all positive strings.  

The `Benchmarks` directory contains all datasets used to evaluate performance, while the `Scripts` directory holds the Python scripts that generate them.

## Build

### Dependencies

* CMake v3.24+
* C++17 Compiler
* CUDA 11.5+

### CMake Options

The following options are used to change the build configuration

#### EVALUATION_MODE

Split the data set to train and test set with a given ratio, and return the precision, recall and f1-score

* `ON`
* `OFF`

*Default:* `OFF`

#### PROFILE_MODE

Add the compile flags needed for **Nsight Compute**

* `ON`
* `OFF`

*Default:* `OFF`

#### LOG_LEVEL

A higher log level, such as `REI_KERNELS`, includes all the levels below it.

* `OFF`
* `DC` - Log information on diviad and conqure level
* `REI_BASIC` - Log an overview for every PaRESy call
* `REI_KERNELS` - Log in-detail for every PaRESy call

*Default:* `OFF`

#### RELAX_UNIQUENESS_CHECK_TYPE

The relax uniqueness check type.

* `1`
* `2`
* `3`

*Default:* `2`

#### CS_BIT_COUNT

The number of bits in the characteristic sequence.

* `128`
* `256`
* `512`
* `1024`
* `2048`
* `4096`

*Default:* `2048`

### Build  Instructions

**Clone the repository**:
   ```bash
   git clone --recursive https://github.com/Al-Asl/RegexInference
   cd RegexInference/Paresy-S
   ```

**Create a build directory and run CMake**

   ```bash
mkdir build
cd build
cmake ..
make
   ```

## Colab Notebook

This work is provided as a Google Colab notebook, which automatically clones this GitHub repository. You can execute the scripts by using the provided buttons and modifying the inputs as needed.

To access the notebook, please use the link below and follow the instructions provided.

- [notebook link](https://colab.research.google.com/github/Al-Asl/RegexInference/blob/main/Paresy_S.ipynb)
