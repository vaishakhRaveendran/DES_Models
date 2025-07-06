# DES Models

## Table of Contents
- [Overview](#overview)
- [Directory Structure](#directory-structure)
- [Running the Application](#running-the-application)
  - [1. Prepare the DAG Files](#1-prepare-the-dag-files)
  - [2. Run for Testing](#2-run-for-testing)
  - [3. Run Full Experiment](#3-run-full-experiment)
- [Important Notes](#important-notes)

## Overview

This application works with Directed Acyclic Graph (DAG) files for discrete event simulation models. The system supports multiple DAG datasets including stencil, laplace, and gaussian configurations.

## Directory Structure

```
model/
├── stencil/
│   ├── st1.dag
│   ├── st2.dag
│   ├── st3.dag
│   ├── st4.dag
│   └── st5.dag
├── laplace/
│   ├── lp1.dag
│   ├── lp2.dag
│   └── ...
├── gaussian/
│   ├── gs1.dag
│   └── ...
└── models/          # Working directory for active DAG set
```

## Running the Application

### 1. Prepare the DAG Files

Before running any test or experiment, you need to populate the `models/` directory with a **single set of DAG files**.

#### Copy DAG Files
Choose one complete DAG set and copy it to the working directory:

```bash
# Example: Copy stencil DAG set
cp model/stencil/*.dag models/
```

#### Available DAG Sets
- **stencil**: `st1.dag` to `st5.dag`
- **laplace**: `lp1.dag`, `lp2.dag`, etc.
- **gaussian**: `gs1.dag`, etc.

### 2. Run for Testing

To perform a basic test run with heuristic scheduling:

```bash
./run.sh
```

**Interactive Usage:**
- When the application starts, press the `+` key to select the heuristic scheduling option from the menu

### 3. Run Full Experiment

To execute the complete experimental flow (time-consuming, evaluates all configurations):

```bash
./test.sh
```

This will run the full experiment using all DAGs present in the `models/` directory.

## Important Notes

⚠️ **Critical Requirements:**
- The `models/` directory must contain **only one dataset at a time**
- Ensure all files from the previous dataset are removed before copying a new set
- Each DAG set should be complete (e.g., `st1.dag` to `st5.dag` for stencil)

✅ **Best Practices:**
- Always verify the `models/` directory contents before running experiments
- Use the testing option (`./run.sh`) before running full experiments
- Keep track of which DAG set you're currently working with

## Workflow Summary

1. **Clean** the `models/` directory
2. **Copy** the desired DAG set to `models/`
3. **Verify** the correct files are in place
4. **Run** either `./run.sh` (testing) or `./test.sh` (full experiment)
5. **Repeat** for different DAG sets as needed
