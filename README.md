# DES_Models
🔧 Running the Application
1. Prepare the DAG Files

Before running any test or experiment, you need to populate the models/ directory with a single set of DAG files.

Currently, your DAG sets are organized like this:

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

To run the application:

    Copy one complete DAG set (e.g., stencil) into the models/ directory:

    cp model/stencil/*.dag models/

Make sure models/ contains only one dataset at a time (like st1.dag to st5.dag) before running.
2. Run for Testing

To do a basic test run with heuristic scheduling:

./run.sh

    When the application starts, press the + key to select the heuristic scheduling option from the menu.

3. Run Full Experiment

To run the full experiment (time-consuming, evaluates all configurations):

./test.sh

This will run the configured experimentation flow using the DAGs present in models/.
