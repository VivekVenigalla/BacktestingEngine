import os
import json
import glob
from datetime import datetime

# --- Directory Structures ---
BASE_DIR = os.path.dirname(os.path.abspath(__file__)) # Assuming src/
ROOT_DIR = os.path.dirname(BASE_DIR)
CONFIG_DIR = os.path.join(ROOT_DIR, "config")
BATCH_DIR = os.path.join(CONFIG_DIR, "batchConfig")
OUTPUT_DIR = os.path.join(ROOT_DIR, "output")

# --- Global State Registers (Exposed to GUI) ---
state = {
    "registered_accounts": [],
    "registered_brokers": [],
    "registered_feeds": [],
    "historical_batches": []
}

def initialize_environment():
    """Builds required directories and mock global config files if missing."""
    for d in [CONFIG_DIR, BATCH_DIR, OUTPUT_DIR]:
        os.makedirs(d, exist_ok=True)
        
    mock_files = {
        "accounts.json": [{"id": "basicAccount", "initial_balance": 10000, "reset": True}],
        "brokers.json": [{"id": "basicBroker", "commission_rate": 1, "slippage_rate": 0.0005, "account_link": "basicAccount", "reset": True}],
        "data_feeds.json": [{"id": "AAPL_1D", "ticker": "AAPL", "timeframe": "1D", "cagr_length": 10, "csv_filepath": "../data/AAPL_1D.csv"}]
    }
    
    for filename, default_data in mock_files.items():
        filepath = os.path.join(CONFIG_DIR, filename)
        if not os.path.exists(filepath):
            with open(filepath, 'w') as f:
                json.dump(default_data, f, indent=4)

def load_registers():
    """Loads shared registers and historical batch list into the state dictionary."""
    initialize_environment()
    
    # Load Global Configurations
    try:
        with open(os.path.join(CONFIG_DIR, "accounts.json"), 'r') as f:
            state["registered_accounts"] = json.load(f)
        with open(os.path.join(CONFIG_DIR, "brokers.json"), 'r') as f:
            state["registered_brokers"] = json.load(f)
        with open(os.path.join(CONFIG_DIR, "data_feeds.json"), 'r') as f:
            state["registered_feeds"] = json.load(f)
    except Exception as e:
        print(f"Registry load error: {e}")

    # Load Historical Batches from batchConfig folder
    state["historical_batches"] = []
    batch_files = glob.glob(os.path.join(BATCH_DIR, "*.json"))
    for bf in batch_files:
        filename = os.path.basename(bf)
        mod_time = datetime.fromtimestamp(os.path.getmtime(bf)).strftime("%Y-%m-%d %H:%M")
        state["historical_batches"].append({"batch_id": filename.replace('.json', ''), "timestamp": mod_time})

def save_batch_config(batch_payload):
    """Dumps the active GUI batch schema into the batchConfig folder."""
    batch_id = batch_payload.get("simulation_metadata", {}).get("batch_id", "default_batch")
    filepath = os.path.join(BATCH_DIR, f"{batch_id}.json")
    
    with open(filepath, 'w') as f:
        json.dump(batch_payload, f, indent=4)
    
    print(f"✔️ Batch saved to: {filepath}")
    load_registers() # Refresh history
    return filepath

# Initialize on import
load_registers()