# Healthor: Protecting the Weak in Heterogeneous DLTs with Health-aware Flow Control

Healthor is a novel heterogeneity-aware flow-control mechanism for DLT networks with unstructured P2P overlay network. It formalizes existing heterogeneity in a notion of health of a node. Sending rates to neighbors are adjusted individually by by adhering to reported health states. This enables high-end nodes to protect weaker nodes from wasting unnecessary processing power, rapidly changing network load and bursts so that every node can stay in sync and actively participate in consensus.

This repository contains an OMNeT++-based simulator written in C++ and a Python framework to analyze and plot results. 

## How to run

**Prerequisites:**
- [OMNeT++ 5.6.2](https://omnetpp.org/download/)
- Python 3

```bash
git clone https://github.com/jonastheis/healthor.git

# compile simulation
cd healthor/simulation
make

# set up Python venv
cd ../plot
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

# run simulation, collect data & create plots
python main.py
```

The simulation to be run can be configured in `main.py` by setting the parameters for `config_name` and `network_name` accordingly. The available configurations can be found in `simulation/omnetpp.ini`. 


## License

This project is licensed under the [Apache Software License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).

See [`LICENSE`](LICENSE) for more information.

    Copyright 2020 Jonas Theis

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.