import dearpygui.dearpygui as dpg
import core  # Safe import of core backend state and functions
from csv_download import downloadData
from datetime import datetime
import math
import yfinance as yf
import os

dpg.create_context()

#active batch with a deafult id
active_batch = {
    "simulation_metadata": {"batch_id": "batch_123", "notes": ""},
    "account": [],
    "broker": [],
    "data_feeds": [],
    "simulations": []
}

windows = ["landing_hub_window", "workbench_window", "config_manager_window"]

# =============================================================
# HELPER FUNCTIONS
# =============================================================

#functions for going between windows and also resizing
def route_to_view(target_window_tag):
    for view in windows:
        if dpg.does_item_exist(view):
            dpg.configure_item(view, show=(view == target_window_tag))
def resize_windows_handler():
    w = max(dpg.get_viewport_width() - 16, 400)
    h = max(dpg.get_viewport_height() - 39, 300)
    for screen in windows:
        if dpg.does_item_exist(screen):
            dpg.configure_item(screen, width=w, height=h, pos=[0, 0])

#close window
def close_modal(modal_tag):
    if dpg.does_item_exist(modal_tag):
        dpg.delete_item(modal_tag)

#create a message window for errors or messages
def spawn_message_modal(title, message, is_error=False, confirm_callback=None, callback_data=None, need_ok = False):
    #generate a unique integer ID
    modal_tag = dpg.generate_uuid()
    
    #use a conditional statement to change the color if there is a error
    text_color = [255, 100, 100] if is_error else [255, 255, 255]
    
    with dpg.window(label=title, tag=modal_tag, modal=True, show=True, no_collapse=True, no_resize=True, width=350):
        dpg.add_text(message, color=text_color, wrap=330)
        dpg.add_spacer(height=10)
        dpg.add_separator()
        dpg.add_spacer(height=5)
        
        with dpg.group(horizontal=True):
            #if we need a confirmation create confirmation buttons and cancel
            if confirm_callback:
                def wrapper_yes():
                    confirm_callback(callback_data)
                    dpg.delete_item(modal_tag)
                    
                dpg.add_button(label="Yes, Proceed", callback=wrapper_yes, width=160)
                dpg.add_button(label="Cancel", callback=lambda: dpg.delete_item(modal_tag), width=160)
            elif is_error or need_ok:
                #error notice message
                dpg.add_spacer(width=135)
                dpg.add_button(label="OK", callback=lambda: dpg.delete_item(modal_tag), width=60)


    vp_width = dpg.get_viewport_client_width()
    vp_height = dpg.get_viewport_client_height()
    dpg.set_item_pos(modal_tag, [vp_width // 2 - 175, vp_height // 2 - 75])
    return modal_tag

# =============================================================
# CONFIG MODALS
# =============================================================
def spawn_create_account_modal(sender=None, app_data=None, user_data=None):
    close_modal("modal_create_account")
    is_edit = user_data is not None
    data = user_data
    init_id = data["id"] if is_edit else "newAccount"
    init_bal = data["initial_balance"] if is_edit else 10000.0
    init_reset = data["reset"] if is_edit else True

    with dpg.window(label="Create New Global Account", tag="modal_create_account", modal=True, width=350, height=200):
        dpg.add_input_text(label="Account ID", tag="m_acct_id", default_value=init_id)
        dpg.add_input_float(label="Initial Balance", tag="m_acct_bal", default_value=init_bal)
        dpg.add_checkbox(label="Reset", tag="m_acct_reset", default_value=init_reset)
        
        def save():
            new_acc = {
                "id": dpg.get_value("m_acct_id"),
                "initial_balance": dpg.get_value("m_acct_bal"),
                "reset": dpg.get_value("m_acct_reset")
            }
            close_modal("modal_create_account")
            #split frame waits for the close call to finish
            dpg.split_frame()
            logger_id = spawn_message_modal("Logger", "Checking for errors")

            ids = []
            for d in core.state["registered_accounts"]["account"]:
                ids.append(d["id"])
            #if id already exists
            if new_acc["id"] in ids and not is_edit:
                close_modal(logger_id)
                dpg.split_frame()
                spawn_message_modal("ID exists", "Account already exists", is_error=True)
                return

            if new_acc["initial_balance"] < 0.0:
                close_modal(logger_id)
                dpg.split_frame()
                spawn_message_modal("Balance Error", "Invalid balance", is_error=True)
                return
            #if it is a edit, find the account and save the new_acc into the config
            if is_edit:
                #find the account with the id and fill in the new_acc with a enumerate loop
                for idx, a in enumerate(core.state["registered_accounts"]["account"]):
                    if a["id"] == new_acc["id"]:
                        core.state["registered_accounts"]["account"][idx] = new_acc
                        core.update_account_config()
            else:
                core.save_account_config(new_acc)
            refresh_all_ui()
            close_modal(logger_id)
            dpg.split_frame()
            logger_id = spawn_message_modal("Logger", f"Account {new_acc["id"]} saved", need_ok = True)  
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_create_account"))

def spawn_create_broker_modal(sender=None, app_data=None, user_data=None):
    close_modal("modal_create_broker")
    account_ids = []
    for a in core.state["registered_accounts"]["account"]:
        account_ids.append(a["id"])
    data = user_data
    is_edit = data is not None
    init_id = data["id"] if is_edit else "newBroker"
    init_commission = data["commission_rate"] if is_edit else 1.0
    init_slippage = data["slippage_rate"] if is_edit else 0.0005
    #init_account = data["account_link"] if is_edit else account_ids[0]
    init_reset = data["reset"] if is_edit else True

    with dpg.window(label="Create New Global Broker", tag="modal_create_broker", modal=True, width=380, height=240):
        dpg.add_input_text(label="Broker ID", tag="m_brk_id", default_value=init_id)
        dpg.add_input_float(label="Commission Rate", tag="m_brk_comm", default_value=init_commission)
        dpg.add_input_float(label="Slippage Rate", tag="m_brk_slip", default_value=init_slippage)
        #dpg.add_combo(label="Account Link", tag="m_brk_link", items = account_ids, default_value=init_account)
        dpg.add_checkbox(label="Reset", tag="m_brk_reset", default_value=init_reset)
        
        def save():
            new_brk = {
                "id": dpg.get_value("m_brk_id"),
                "commission_rate": dpg.get_value("m_brk_comm"),
                "slippage_rate": dpg.get_value("m_brk_slip"),
                #"account_link": dpg.get_value("m_brk_link"),
                "reset": dpg.get_value("m_brk_reset")
            }
            close_modal("modal_create_broker")
            #split frame waits for the close call to finish
            dpg.split_frame()
            logger_id = spawn_message_modal("Logger", "Checking for errors")

            ids = []
            for d in core.state["registered_brokers"]["broker"]:
                ids.append(d["id"])
            #if id already exists
            if new_brk["id"] in ids and not is_edit:
                close_modal(logger_id)
                dpg.split_frame()
                spawn_message_modal("ID exists", "Broker already exists", is_error=True)
                return

            if new_brk["commission_rate"] < 0.0:
                close_modal(logger_id)
                dpg.split_frame()
                spawn_message_modal("Commission Error", "Invalid commission rate", is_error=True)
                return

            if new_brk["slippage_rate"] < 0.0:
                close_modal(logger_id)
                dpg.split_frame()
                spawn_message_modal("Slippage Error", "Invalid slippage rate", is_error=True)
                return

            if is_edit:
                for idx, b in enumerate(core.state["registered_brokers"]["broker"]):
                    if b["id"] == new_brk["id"]:
                        core.state["registered_brokers"]["broker"][idx] = new_brk
                        core.update_broker_config()
            else:
                core.save_broker_config(new_brk)


            refresh_all_ui()
            close_modal(logger_id)
            dpg.split_frame()
            logger_id = spawn_message_modal("Logger", f"Broker {new_brk["id"]} saved", need_ok = True)  
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_create_broker"))

def spawn_create_feed_modal(sender=None, app_data=None, user_data=None):
    close_modal("modal_create_feed")
    is_edit = user_data is not None
    title = "Edit Data Feed Blueprint" if is_edit else "Create New Global Data Feed"
    
    data = user_data
    init_tick = data["ticker"] if is_edit else "MSFT"
    init_tf = data["timeframe"] if is_edit else "1D"
    day1 = datetime.strptime(data["start_date"], "%Y-%m-%d") if is_edit else 0
    day2 = datetime.strptime(data["end_date"], "%Y-%m-%d") if is_edit else 0
    init_start_day = day1.day if is_edit else 1
    init_start_month = (day1.month -1) if is_edit else 0
    init_start_year = (day1.year-1900) if is_edit else 124
    init_end_day = day2.day if is_edit else 31
    init_end_month = (day2.month -1) if is_edit else 11
    init_end_year = (day2.year-1900) if is_edit else 124


    with dpg.window(label="Create New Global Data Feed", tag="modal_create_feed", modal=True, width=420, height=520):
        #dpg.add_input_text(label="Feed ID", tag="m_fd_id", default_value="MSFT_1D")
        dpg.add_input_text(label="Ticker", tag="m_fd_tick", default_value=init_tick)
        dpg.add_combo(label = "Timeframe", tag = "m_fd_tf", items = ["1D","5D","1MO","3MO","1WK","1H"], default_value = init_tf)
        #dpg.add_input_text(label="Timeframe", tag="m_fd_tf", default_value="1D")
        #dpg.add_input_int(label="CAGR Length", tag="m_fd_cagr", default_value=10)
        #dpg.add_input_text(label="CSV Filepath", tag="m_fd_csv", default_value="../data/MSFT_1D.csv")
        dpg.add_text("Start Date:")
        dpg.add_date_picker(
            tag="m_fd_start", 
            level=dpg.mvDatePickerLevel_Day, 
            default_value={'month_day': init_start_day, 'month': init_start_month, 'year': init_start_year} #year is 2024 and month is 0 indexed
        )
        
        dpg.add_text("End Date:")
        dpg.add_date_picker(
            tag="m_fd_end", 
            level=dpg.mvDatePickerLevel_Day, 
            default_value={'month_day': init_end_day, 'month': init_end_month, 'year': init_end_year}
        )
        def save():
            check = True
            raw_start = dpg.get_value("m_fd_start")
            raw_end = dpg.get_value("m_fd_end")
            
            #format since dear pygui starts years since 1900 and uses 0 indexed month
            start_str = f"{raw_start['year'] + 1900:04d}-{raw_start['month'] + 1:02d}-{raw_start['month_day']:02d}"
            end_str = f"{raw_end['year'] + 1900:04d}-{raw_end['month'] + 1:02d}-{raw_end['month_day']:02d}"

            #to get cagr length we need to parse the string as a datetime object and then use that to find the percentage of a year
            d1 = datetime.strptime(start_str, "%Y-%m-%d")
            d2 = datetime.strptime(end_str, "%Y-%m-%d")
            days_between = abs((d2 - d1).days)


            #use math.ceil to use only integers for the length of years elapsed
            cagr_length = math.ceil((days_between/365.2425))
            new_fd = {
                "id": dpg.get_value("m_fd_tick") + "_" + dpg.get_value("m_fd_tf") +  "_" +  start_str + "_" + end_str,
                "ticker": dpg.get_value("m_fd_tick"),
                "timeframe": dpg.get_value("m_fd_tf"),
                "start_date": start_str,
                "end_date": end_str,
                "cagr_length": cagr_length,
                "csv_filepath": "../data/" + dpg.get_value("m_fd_tick") + "_" + dpg.get_value("m_fd_tf") +  "_" + start_str + "_" + end_str + ".csv"
            }

            #now that we have all the values, we can close this modal and create a saving modal
            close_modal("modal_create_feed")
            dpg.split_frame()
            logger_id = spawn_message_modal("Logger", "Checking for errors")
            #obtain all ids
            ids = []
            for d in core.state["registered_feeds"]["data_feeds"]:
                ids.append(d["id"])
            #if id already exists
            if new_fd["id"] in ids:
                close_modal(logger_id)
                dpg.split_frame()
                spawn_message_modal("ID exists", "Feed already exists", is_error=True)
                return

            if(d2 <= d1):
                close_modal(logger_id)
                
                dpg.split_frame()
                
                spawn_message_modal("Invalid Date Range", "End Date must be after Start Date", is_error=True)
                return

            #check if the ticker is valid
            try:
                checkTicker = yf.Ticker(new_fd["ticker"])
                # Attempt to download a tiny sliver of historical data
                hist = checkTicker.history(period="1d")
                
                # If the ticker is fake, the resulting dataframe will be empty
                if hist.empty:
                    close_modal(logger_id)
                
                    dpg.split_frame()
                    
                    spawn_message_modal("Ticker Wrong", f"{new_fd["ticker"]} is an invalid ticker", is_error=True)
                    return
                
            except Exception:
                # Catches 404 client errors or network failures
                close_modal(logger_id)
                
                dpg.split_frame()
                
                spawn_message_modal("Invalid Ticker", f"{new_fd["ticker"]} is an invalid ticker", is_error=True)
                return

            close_modal(logger_id)
            dpg.split_frame()
            logger_id = spawn_message_modal("Logger", "Saving feed")

            #if there are no error messages in the creation of the feed, create it
            try:
                if is_edit:
                    for idx, f in enumerate(core.state["registered_feeds"]["data_feeds"]):
                        if f["id"] == user_data["id"]:
                            core.state["registered_feeds"]["data_feeds"][idx] = new_fd
                            core.update_feed_config()
                            data_dir = os.path.join("..", "data")
                            os.makedirs(data_dir, exist_ok=True)
                            tempFile = user_data["csv_filepath"]
                            tempFile = os.path.join(data_dir, tempFile)

                            close_modal(logger_id)
                            dpg.split_frame()
                            logger_id = spawn_message_modal("Logger", f"Edited {f["id"]} in feedConfig.json. Deleting {tempFile}...")

                            if os.path.exists(tempFile):
                                os.remove(tempFile)
                                print("File deleted successfully.")
                                close_modal(logger_id)
                                dpg.split_frame()
                                logger_id = spawn_message_modal("Logger", f"Deleted {tempFile} . Downloading {new_fd["csv_filepath"]} ...")
                                
                else:
                    close_modal(logger_id)
                    dpg.split_frame()
                    logger_id = spawn_message_modal("Logger", f"Saved {new_fd["id"]} to feedCongig.json. Downloading {new_fd["csv_filepath"]} ...")
                    core.save_feed_config(new_fd)


                
                print("downloading data")
                downloadData(new_fd["ticker"], new_fd["start_date"], new_fd["end_date"], new_fd["timeframe"])
                refresh_all_ui()
                close_modal(logger_id)
                dpg.split_frame()
                logger_id = spawn_message_modal("Logger", f"Downloaded {new_fd["csv_filepath"]} ...", need_ok = True)         
                
            except Exception as e:
                close_modal(logger_id)
                dpg.split_frame()
                spawn_message_modal("Download Error", f"Error: {repr(e)}", is_error=True)
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_create_feed"))

def delete_account(account_id):
    core.perform_delete_account(account_id)
    refresh_all_ui()

def delete_broker(broker_id):
    core.perform_delete_broker(broker_id)
    refresh_all_ui()

def delete_feed(feed_id):
    core.perform_delete_feed(feed_id)
    refresh_all_ui()

# =============================================================
# BATCH AND CONFIG MANAGEMENT
# =============================================================
def add_entity_to_batch(sender, app_data, user_data):
    entity_type, entity_data = user_data
    if entity_type == "ACCOUNT":
        # Check if already present to avoid duplicates
        if not any(a["id"] == entity_data["id"] for a in active_batch["account"]):
            active_batch["account"].append(dict(entity_data))
    elif entity_type == "BROKER":
        if not any(b["id"] == entity_data["id"] for b in active_batch["broker"]):
            active_batch["broker"].append(dict(entity_data))
    elif entity_type == "FEED":
        if not any(f["id"] == entity_data["id"] for f in active_batch["data_feeds"]):
            active_batch["data_feeds"].append(dict(entity_data))
            
    print(f"[UI] Added {entity_data['id']} to batch {entity_type} register.")

#refresh sidepane in view 3
def refresh_registry_sidepane():
    #accounts
    if dpg.does_item_exist("reg_accounts_container"):
        dpg.delete_item("reg_accounts_container", children_only=True)
        acct_list = core.state["registered_accounts"].get("account", [])
        for acct in acct_list:
            dpg.add_button(
                label=f"+ {acct['id']} (${acct['initial_balance']})", 
                parent="reg_accounts_container", 
                user_data=("ACCOUNT", acct), 
                callback=add_entity_to_batch,
                width=-1
            )

    #brokers
    if dpg.does_item_exist("reg_brokers_container"):
        dpg.delete_item("reg_brokers_container", children_only=True)
        brk_list = core.state["registered_brokers"].get("broker", [])
        for brk in brk_list:
            dpg.add_button(
                label=f"+ {brk['id']}", 
                parent="reg_brokers_container", 
                user_data=("BROKER", brk), 
                callback=add_entity_to_batch,
                width=-1
            )

    #feeds
    if dpg.does_item_exist("reg_feeds_container"):
        dpg.delete_item("reg_feeds_container", children_only=True)
        fd_list = core.state["registered_feeds"].get("data_feeds", [])
        for fd in fd_list:
            dpg.add_button(
                label=f"+ {fd['id']} ({fd['ticker']})", 
                parent="reg_feeds_container", 
                user_data=("FEED", fd), 
                callback=add_entity_to_batch,
                width=-1
            )

     #strategues
    if dpg.does_item_exist("reg_strategies_container"):
        dpg.delete_item("reg_strategies_container", children_only=True)
        strat_list = core.state.get("registered_strategies", {}).get("strategies", [])
        for st in strat_list:
            dpg.add_button(
                label=f"+ {st['display_name']} Node", 
                parent="reg_strategies_container", 
                user_data=("STRATEGY", st), 
                callback=add_entity_to_batch,
                width=-1
            )

#refresh strategy table
def refresh_strategy_table():
    if dpg.does_item_exist("table_strategies"):
        #clear rows
        for item in dpg.get_item_children("table_strategies", 1):
            dpg.delete_item(item)
            
        strategies = core.state.get("registered_strategies", {}).get("strategies", [])
        for strat in strategies:
            with dpg.table_row(parent="table_strategies"):
                dpg.add_text(strat["id"])
                dpg.add_text(strat.get("display_name", strat["id"]))
                dpg.add_text(strat.get("description", "No description provided."))
                
                #format parameters
                params_str = ", ".join(
                    [f"{p['name']} ({p['type']}={p['default']})" for p in strat.get("parameters", [])]
                )
                dpg.add_text(params_str if params_str else "None")

#refresh config table
def refresh_config_manager_tables():
    #Account Table
    if dpg.does_item_exist("table_accounts"):
        #delete thr rows only
        for item in dpg.get_item_children("table_accounts", 1):
            dpg.delete_item(item)
            
        for acct in core.state["registered_accounts"].get("account", []):
            with dpg.table_row(parent="table_accounts"):
                dpg.add_text(acct["id"])
                dpg.add_text(f"${acct['initial_balance']}")
                dpg.add_text(str(acct["reset"]))
                with dpg.group(horizontal=True):
                    dpg.add_button(label="Edit", callback=spawn_create_account_modal, user_data=acct)
                    dpg.add_button(
                        label="Delete", 
                        callback=lambda s, a, u: spawn_message_modal(
                            "Delete Account", 
                            f"Delete account '{u}'?", 
                            is_error=True,
                            confirm_callback=delete_account, 
                            callback_data=u
                        ), 
                        user_data=acct["id"]
                    )


    #Broker Table
    if dpg.does_item_exist("table_brokers"):
        for item in dpg.get_item_children("table_brokers", 1):
            dpg.delete_item(item)
            
        for brk in core.state["registered_brokers"].get("broker", []):
            with dpg.table_row(parent="table_brokers"):
                dpg.add_text(brk["id"])
                #dpg.add_text(brk.get("account_link", "N/A"))
                dpg.add_text(f"{brk['commission_rate']} / {brk['slippage_rate']}")
                dpg.add_text(str(brk["reset"]))
                with dpg.group(horizontal=True):
                    dpg.add_button(label="Edit", callback=spawn_create_broker_modal, user_data=brk)
                    dpg.add_button(
                        label="Delete", 
                        callback=lambda s, a, u: spawn_message_modal(
                            "Delete Broker", 
                            f"Delete broker '{u}'?", 
                            is_error=True,
                            confirm_callback=delete_broker, 
                            callback_data=u
                        ), 
                        user_data=brk["id"]
                    )

    #Feed Table
    if dpg.does_item_exist("table_feeds"):
        for item in dpg.get_item_children("table_feeds", 1):
            dpg.delete_item(item)
            
        for fd in core.state["registered_feeds"].get("data_feeds", []):
            with dpg.table_row(parent="table_feeds"):
                dpg.add_text(fd["id"])
                dpg.add_text(fd["ticker"])
                dpg.add_text(f"{fd.get('start_date', 'N/A')} to {fd.get('end_date', 'N/A')}")
                with dpg.group(horizontal=True):
                    dpg.add_button(label="Edit", callback=spawn_create_feed_modal, user_data=fd)
                    dpg.add_button(
                        label="Delete", 
                        callback=lambda s, a, u: spawn_message_modal(
                            "Delete Data Feed", 
                            f"Delete feed '{u}'?", 
                            is_error=True,
                            confirm_callback=delete_feed, 
                            callback_data=u
                        ), 
                        user_data=fd["id"]
                    )

    refresh_strategy_table()

def refresh_all_ui():
    refresh_registry_sidepane()
    refresh_config_manager_tables()

#themes for entire window
with dpg.theme() as global_theme:
    with dpg.theme_component(dpg.mvAll):
        dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 14, 14, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_FramePadding, 6, 6, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing, 8, 8, category=dpg.mvThemeCat_Core)
dpg.bind_theme(global_theme)

#NOTE in dearpygui, windows are the main canvas, while viewports are the individual sections you see

# =============================================================
# VEIEWPORTS
# =============================================================

# ================================
# VIEWPORT 1: LANDING PAGE
# ================================
with dpg.window(tag="landing_hub_window", no_move=True, no_resize=True, no_title_bar=True, show=True):
    dpg.add_text("Backtesting Engine By Vivek Venigalla", color=[100, 200, 255])
    dpg.add_separator()
    dpg.add_spacer(height=10)
    with dpg.group(horizontal=True):
        #left panel containg quick actions such as creating a batch or manageing global config
        with dpg.child_window(width=320, height=-1):
            dpg.add_button(label="+ Create New Simulation Batch", width=-1, height=50, callback=lambda: route_to_view("workbench_window"))
            dpg.add_spacer(height=10)
            dpg.add_button(label="Manage Global Infrastructure", width=-1, height=50, callback=lambda: route_to_view("config_manager_window"))
            dpg.add_spacer(height = 10)
        #right panel showing recent activity
        with dpg.child_window(width=-1, height=-1):
            dpg.add_text("Historical Batches Directory (From batchConfig/)", color=[140, 140, 140])
            dpg.add_separator()
            if not core.state["historical_batches"]:
                dpg.add_text("No historical configurations found.", color=[255, 165, 0])
            else:
                for batch in core.state["historical_batches"]:
                    dpg.add_text(f"{batch['batch_id']}.json (Modified: {batch['timestamp']})")

# =====================================
# VIEWPORT 2: GLOBAL CONFIG MANAGEMENT
# =====================================

with dpg.window(tag="config_manager_window", no_move=True, no_resize=True, no_title_bar=True, show=False):
    with dpg.group(horizontal=True):
        dpg.add_button(label="Back to Hub", callback=lambda: route_to_view("landing_hub_window"))
        dpg.add_text("GLOBAL CONFIGURATION", color=[100, 200, 255])
    dpg.add_separator()
    dpg.add_spacer(height=10)
    
    with dpg.tab_bar():
        # Accounts Tab
        with dpg.tab(label="Accounts"):
            dpg.add_spacer(height=5)
            dpg.add_button(label="+ Create New Account", callback=spawn_create_account_modal)
            dpg.add_spacer(height=5)
            with dpg.table(tag="table_accounts", header_row=True, borders_innerH=True, borders_outerH=True, borders_innerV=True, borders_outerV=True):
                dpg.add_table_column(label="Account ID")
                dpg.add_table_column(label="Initial Balance")
                dpg.add_table_column(label="Reset Flag")
                dpg.add_table_column(label="Actions", width_fixed=True, init_width_or_weight=300)

        # Brokers Tab
        with dpg.tab(label="Brokers"):
            dpg.add_spacer(height=5)
            dpg.add_button(label="+ Create New Broker", callback=spawn_create_broker_modal)
            dpg.add_spacer(height=5)
            with dpg.table(tag="table_brokers", header_row=True, borders_innerH=True, borders_outerH=True, borders_innerV=True, borders_outerV=True):
                dpg.add_table_column(label="Broker ID")
                #dpg.add_table_column(label="Linked Account")
                dpg.add_table_column(label="Commission / Slippage")
                dpg.add_table_column(label="Reset Flag")
                dpg.add_table_column(label="Actions", width_fixed=True, init_width_or_weight=300)

        # Data Feeds Tab
        with dpg.tab(label="Data Feeds"):
            dpg.add_spacer(height=5)
            dpg.add_button(label="+ Create New Feed", callback=spawn_create_feed_modal)
            dpg.add_spacer(height=5)
            with dpg.table(tag="table_feeds", header_row=True, borders_innerH=True, borders_outerH=True, borders_innerV=True, borders_outerV=True):
                dpg.add_table_column(label="Feed ID")
                dpg.add_table_column(label="Ticker")
                dpg.add_table_column(label="Date Range")
                dpg.add_table_column(label="Actions", width_fixed=True, init_width_or_weight=300)

        # Strategies Blueprint Tab (Immutable)
        with dpg.tab(label="Strategies"):
            dpg.add_spacer(height=5)
            dpg.add_text(
                "Strategy Blueprints(Read Only). These correspond to C++ files in the /strategies folder."
                "Parameters can be customized when added to the Node Workbench.",
                color=[180, 180, 180],
                wrap=600
            )
            dpg.add_spacer(height=5)
            with dpg.table(tag="table_strategies", header_row=True, borders_innerH=True, borders_outerH=True, borders_innerV=True, borders_outerV=True):
                dpg.add_table_column(label="Strategy ID", width_fixed=True, init_width_or_weight=100)
                dpg.add_table_column(label="Display Name", width_fixed=True, init_width_or_weight=200)
                dpg.add_table_column(label="Description")
                dpg.add_table_column(label="Default Parameters")

# ==============================
# VIEWPORT 3: BATCH MANEGEMENT
# ==============================
with dpg.window(tag="workbench_window", no_move=True, no_resize=True, no_title_bar=True, show=False):
    with dpg.group(horizontal=True):
        dpg.add_button(label="Back to Hub", callback=lambda: route_to_view("landing_hub_window"))
        dpg.add_text("WORKBENCH PIPELINE MANAGEMENT", color=[100, 200, 255])
    dpg.add_separator()
    dpg.add_spacer(height=10)
    
    with dpg.group(horizontal=True):
        #All nodes for the batch config
        with dpg.child_window(width=360, height=-1):
            dpg.add_input_text(label="Batch ID", tag="ui_batch_id", default_value=active_batch["simulation_metadata"]["batch_id"])
            dpg.add_input_text(label="Notes", tag="ui_batch_notes", default_value=active_batch["simulation_metadata"]["notes"])
            dpg.add_spacer(height=10)
            
            dpg.add_text("Node Library", color=[100, 255, 100])
            dpg.add_separator()
            
            with dpg.collapsing_header(label="Accounts"):
                with dpg.group(tag="reg_accounts_container"): pass
                
            with dpg.collapsing_header(label="Brokers"):
                with dpg.group(tag="reg_brokers_container"): pass
                
            with dpg.collapsing_header(label="Data Feeds"):
                with dpg.group(tag="reg_feeds_container"): pass

            with dpg.collapsing_header(label="Strategies"):
                with dpg.group(tag="reg_strategies_container"): pass

        #node editor
        with dpg.child_window(width=-1, height=-1):
            with dpg.node_editor(tag="node_editor_canvas"):
                pass

# =============================================================
# EXECUTION
# =============================================================
dpg.create_viewport(title='Backtesting Engine', width=1300, height=840)
dpg.setup_dearpygui()
dpg.show_viewport()

dpg.set_viewport_resize_callback(resize_windows_handler)
resize_windows_handler()

# Render side-pane contents from core.state safely
refresh_all_ui()

dpg.start_dearpygui()
dpg.destroy_context()