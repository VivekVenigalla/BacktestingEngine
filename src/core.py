import os
import json
import glob
from datetime import datetime

#Get the important directories
#dir name returns the parent directory
BASE_DIR = os.path.dirname(os.path.abspath(__file__)) #src/
ROOT_DIR = os.path.dirname(BASE_DIR) #BacktestingEngine/
CONFIG_DIR = os.path.join(ROOT_DIR, "config") #config/
BATCH_DIR = os.path.join(CONFIG_DIR, "batchConfig")
OUTPUT_DIR = os.path.join(ROOT_DIR, "output")
DATA_DIR = os.path.join(ROOT_DIR, "data")

#global variables that are accessed by gui
state = {
    "registered_accounts": [],
    "registered_brokers": [],
    "registered_feeds": [],
    "registered_strategies": [],
    "historical_batches": []
}

def check_environment():
    #ensure that all paths exist and creates if necessary(useful in case of accidental deletions)
    for path in [CONFIG_DIR, BATCH_DIR, OUTPUT_DIR, DATA_DIR]:
        os.makedirs(path, exist_ok=True)
        
    #in the event a config file does not exist make one with these default values
    default_blueprints = {
        "accountConfig.json": {
            "account" : [
                {"id": "basicAccount", "initial_balance": 10000, "reset": True},
                {"id":"bollAccount","initial_balance":10000,"reset":True},
                {"id":"donAccount","initial_balance":10000,"reset":True}
            ]
        },
        "brokerConfig.json": {
            "broker" : [
                {"id": "basicBroker", "commission_rate": 1.0, "slippage_rate": 0.0005, "account_link": "basicAccount", "reset": True}
            ]
        },
        "feedConfig.json": {
            "data_feeds" : [
                {"id": "AAPL_1D", "ticker": "AAPL", "timeframe": "1D", "cagr_length": 10, "csv_filepath": "../data/AAPL_1D.csv"}
            ]
        },
        "strategyBlueprints.json":{
            "strategies":[
                {
                    "id": "sma",
                    "display_name": "Simple Moving Average Crossover",
                    "description": "Trend following strategy using short and long SMA crossing.",
                    "default_params": {"fast_period": 10, "slow_period": 50}
                },
                {
                    "id": "don",
                    "display_name": "Donchian Channel",
                    "description": "Breakout strategy using stop orders on extreme values.",
                    "default_params": {"window": 20}
                },
                {
                    "id": "boll",
                    "display_name": "Bollinger Band",
                    "description": "Trend following strategy utilizing standard deviations and crossings.",
                    "default_params": {"window": 20}
                }
            ]
        }
    }
    
    #iterate over the config files and create if needed
    for filename, structure in default_blueprints.items():
        #create full path
        target_path = os.path.join(CONFIG_DIR, filename)
        if not os.path.exists(target_path):
            #opens file
            with open(target_path, 'w') as f:
                #json.dump literally dumps the dictionary
                json.dump(structure, f, indent=4)

#reload the registers for the gui
def reload_registers():
    #make sure the directories exist first
    check_environment()
    
    #get the objects
    try:
        #accounts
        with open(os.path.join(CONFIG_DIR, "accountConfig.json"), 'r') as f:
            state["registered_accounts"] = json.load(f)
        #broker
        with open(os.path.join(CONFIG_DIR, "brokerConfig.json"), 'r') as f:
            state["registered_brokers"] = json.load(f)
        #feeds
        with open(os.path.join(CONFIG_DIR, "feedConfig.json"), 'r') as f:
            state["registered_feeds"] = json.load(f)
        with open(os.path.join(CONFIG_DIR, "strategyBlueprints.json"), 'r') as f:
            state["registered_strategies"] = json.load(f)
    except Exception as e:
        print(f"Failed parsing standard registries: {e}")

    #get existing batches

    #empty out the dictionary
    state["historical_batches"] = []
    #obtain all of the json files found in this directory
    #glob.glob allows the directory access for batch access
    found_profiles = glob.glob(os.path.join(BATCH_DIR, "*.json"))
    #iterate over batches for loading
    for file_path in found_profiles:
        #get the filename eg batch_123.json
        raw_name = os.path.basename(file_path)
        #get last modified time for records
        mod_stamp = datetime.fromtimestamp(os.path.getmtime(file_path)).strftime("%Y-%m-%d %H:%M")
        #remove the extension for batchID and save it in the dict with the timestamp
        state["historical_batches"].append({
            "batch_id": raw_name.replace('.json', ''),
            "timestamp": mod_stamp
        })


#save the batch into a file
#NOTE : dict means the variable is a dictionary and -> str means the return type is a string. Useful for debugging
def save_batch_config(batch_payload : dict) -> str:
    #get the batch_id  from the payload(the second parameter is the default value for the get function)
    batch_id = batch_payload.get("simulation_metadata", {}).get("batch_id", "default_batch")
    #create the filepath
    filepath = os.path.join(BATCH_DIR, f"{batch_id}.json")
    
    #open the file if it exists and clear anything
    with open(filepath, 'w') as f:
        #dump the dictionary into the json file
        json.dump(batch_payload, f, indent=4)
    
    #print confirmation and reload registers
    print(f"Batch saved to: {filepath}")
    reload_registers()
    return filepath

#this file requires that reload_registers is called first to have the account info already in store
def update_account_config():
    filepath = os.path.join(CONFIG_DIR, "accountConfig.json")
    with open(filepath, 'w') as f:
        #dump the dictionary into the json file
        json.dump(state["registered_accounts"], f, indent=4)
    print(f"Account saved")
    reload_registers()

def save_account_config(new_account : dict):
    #update the register
    state["registered_accounts"]["account"].append(new_account);

    #create the filepath
    filepath = os.path.join(CONFIG_DIR, "accountConfig.json")
    
    #open the file if it exists and clear anything
    #since we already have the updated dict in state, it is ok if the json is cleared
    with open(filepath, 'w') as f:
        #dump the dictionary into the json file
        json.dump(state["registered_accounts"], f, indent=4)
    
    #print confirmation and reload registers
    print(f"Account saved")
    reload_registers()

def update_broker_config():
    filepath = os.path.join(CONFIG_DIR, "brokerConfig.json")
    with open(filepath, 'w') as f:
        #dump the dictionary into the json file
        json.dump(state["registered_brokers"], f, indent=4)
    print(f"Broker saved")
    reload_registers()

def save_broker_config(new_broker : dict):
    #update the register
    state["registered_brokers"]["broker"].append(new_broker);

    #create the filepath
    filepath = os.path.join(CONFIG_DIR, "brokerConfig.json")
    
    #open the file if it exists and clear anything
    #since we already have the updated dict in state, it is ok if the json is cleared
    with open(filepath, 'w') as f:
        #dump the dictionary into the json file
        json.dump(state["registered_brokers"], f, indent=4)
    
    #print confirmation and reload registers
    print(f"Broker saved")
    reload_registers()

def update_feed_config():
    filepath = os.path.join(CONFIG_DIR, "feedConfig.json")
    with open(filepath, 'w') as f:
        #dump the dictionary into the json file
        json.dump(state["registered_feeds"], f, indent=4)
    print(f"Feed saved")
    reload_registers()

def save_feed_config(new_feed : dict):
    #update the register
    state["registered_feeds"]["data_feeds"].append(new_feed);

    #create the filepath
    filepath = os.path.join(CONFIG_DIR, "feedConfig.json")
    
    #open the file if it exists and clear anything
    #since we already have the updated dict in state, it is ok if the json is cleared
    with open(filepath, 'w') as f:
        #dump the dictionary into the json file
        json.dump(state["registered_feeds"], f, indent=4)
    
    #print confirmation and reload registers
    print(f"Feed saved")
    reload_registers()

def perform_delete_account(account_id):
    state["registered_accounts"]["account"] = [
        a for a in state["registered_accounts"]["account"] if a["id"] != account_id
    ]
    update_account_config()

def perform_delete_broker(broker_id):
    state["registered_brokers"]["broker"] = [
        b for b in state["registered_brokers"]["broker"] if b["id"] != broker_id
    ]
    update_broker_config()

def perform_delete_feed(feed_id):
    matchFeed = next((feed for feed in state["registered_feeds"]["data_feeds"] if feed["id"] == feed_id), None)
    state["registered_feeds"]["data_feeds"] = [
        f for f in state["registered_feeds"]["data_feeds"] if f["id"] != feed_id
    ]
    #update config Json file
    update_feed_config()
    if(matchFeed is not None):
        data_dir = os.path.join("..", "data")
        os.makedirs(data_dir, exist_ok=True)
        tempFile = matchFeed["csv_filepath"]
        tempFile = os.path.join(data_dir, tempFile)
        print(tempFile)
        if os.path.exists(tempFile):
            os.remove(tempFile)
            print("File deleted successfully.")

#when the file is imported this function is automatically runned
reload_registers()

print(state)