import dearpygui.dearpygui as dpg
import core  # Safe import of core backend state and functions
from csv_download import downloadData
from datetime import datetime
import math
import yfinance as yf

dpg.create_context()

# --- Active Batch Memory Structure (Working Context) ---
active_batch = {
    "simulation_metadata": {"batch_id": "batch_123", "notes": ""},
    "account": [],
    "broker": [],
    "data_feeds": [],
    "simulations": []
}

# --- Screen Navigation Router ---
def route_to_view(target_window_tag):
    all_screens = ["landing_hub_window", "workbench_window"]
    for view in all_screens:
        if dpg.does_item_exist(view):
            dpg.configure_item(view, show=(view == target_window_tag))

# --- Helper Modal Utility ---
def close_modal(modal_tag):
    if dpg.does_item_exist(modal_tag):
        dpg.delete_item(modal_tag)
#create a message window for errors or messages
def spawn_message_modal(title, message, is_error=False, confirm_callback=None, callback_data=None):
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
            else:
                #error notice message
                dpg.add_spacer(width=135)
                dpg.add_button(label="OK", callback=lambda: dpg.delete_item(modal_tag), width=60)

    vp_width = dpg.get_viewport_client_width()
    vp_height = dpg.get_viewport_client_height()
    dpg.set_item_pos(modal_tag, [vp_width // 2 - 175, vp_height // 2 - 75])

# ==============================================================================
# GLOBAL REGISTRY ADDITION MODALS (Save directly to config/*.json via core)
# ==============================================================================
def spawn_create_account_modal():
    close_modal("modal_create_account")
    with dpg.window(label="Create New Global Account", tag="modal_create_account", modal=True, width=350, height=200):
        dpg.add_input_text(label="Account ID", tag="m_acct_id", default_value="newAccount")
        dpg.add_input_float(label="Initial Balance", tag="m_acct_bal", default_value=10000.0)
        dpg.add_checkbox(label="Reset", tag="m_acct_reset", default_value=True)
        
        def save():
            new_acc = {
                "id": dpg.get_value("m_acct_id"),
                "initial_balance": dpg.get_value("m_acct_bal"),
                "reset": dpg.get_value("m_acct_reset")
            }
            core.save_account_config(new_acc)
            refresh_registry_sidepane()
            close_modal("modal_create_account")
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_create_account"))

def spawn_create_broker_modal():
    close_modal("modal_create_broker")
    account_ids = []
    for a in core.state["registered_accounts"]["account"]:
        account_ids.append(a["id"])
    with dpg.window(label="Create New Global Broker", tag="modal_create_broker", modal=True, width=380, height=240):
        dpg.add_input_text(label="Broker ID", tag="m_brk_id", default_value="newBroker")
        dpg.add_input_float(label="Commission Rate", tag="m_brk_comm", default_value=1.0)
        dpg.add_input_float(label="Slippage Rate", tag="m_brk_slip", default_value=0.0005)
        dpg.add_combo(label="Account Link", tag="m_brk_link", items = account_ids, default_value="basicAccount")
        dpg.add_checkbox(label="Reset", tag="m_brk_reset", default_value=True)
        
        def save():
            new_brk = {
                "id": dpg.get_value("m_brk_id"),
                "commission_rate": dpg.get_value("m_brk_comm"),
                "slippage_rate": dpg.get_value("m_brk_slip"),
                "account_link": dpg.get_value("m_brk_link"),
                "reset": dpg.get_value("m_brk_reset")
            }
            core.save_broker_config(new_brk)
            refresh_registry_sidepane()
            close_modal("modal_create_broker")
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_create_broker"))

def spawn_create_feed_modal():
    close_modal("modal_create_feed")
    with dpg.window(label="Create New Global Data Feed", tag="modal_create_feed", modal=True, width=420, height=520):
        #dpg.add_input_text(label="Feed ID", tag="m_fd_id", default_value="MSFT_1D")
        dpg.add_input_text(label="Ticker", tag="m_fd_tick", default_value="MSFT")
        dpg.add_combo(label = "Timeframe", tag = "m_fd_tf", items = ["1D","5D","1MO","3MO","1WK","1H"], default_value = "1D")
        #dpg.add_input_text(label="Timeframe", tag="m_fd_tf", default_value="1D")
        #dpg.add_input_int(label="CAGR Length", tag="m_fd_cagr", default_value=10)
        #dpg.add_input_text(label="CSV Filepath", tag="m_fd_csv", default_value="../data/MSFT_1D.csv")
        dpg.add_text("Start Date:")
        dpg.add_date_picker(
            tag="m_fd_start", 
            level=dpg.mvDatePickerLevel_Day, 
            default_value={'month_day': 1, 'month': 0, 'year': 124} #year is 2024
        )
        
        dpg.add_text("End Date:")
        dpg.add_date_picker(
            tag="m_fd_end", 
            level=dpg.mvDatePickerLevel_Day, 
            default_value={'month_day': 31, 'month': 11, 'year': 124}
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
            core.save_feed_config(new_fd)
            refresh_registry_sidepane()
            if(d2 <= d1):
                close_modal("modal_create_feed")
                
                dpg.split_frame()
                
                spawn_message_modal("Invalid Date Range", "End Date must be after Start Date", is_error=True)
                check = False



            #if there are no error messages in the creation of the feed, create it
            if check:  
                try:
                    downloadData(new_fd["ticker"], new_fd["start_date"], new_fd["end_date"], new_fd["timeframe"])
                    close_modal("modal_create_feed")
                except Exception as e:
                    #spawn a new message
                    close_modal("modal_create_feed")
                
                    dpg.split_frame()
                    
                    spawn_message_modal("Export Error", f"Something went wrong: {repr(e)}", is_error=True)
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_create_feed"))


# --- Add Entity from Register into Active Batch ---
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

# --- Refresh the Left-Pane Global Entity Lists ---
def refresh_registry_sidepane():
    # Refresh Accounts List
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

    # Refresh Brokers List
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

    # Refresh Data Feeds List
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

# --- Global Style & Window Resizing Handler ---
with dpg.theme() as global_theme:
    with dpg.theme_component(dpg.mvAll):
        dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 14, 14, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_FramePadding, 6, 6, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing, 8, 8, category=dpg.mvThemeCat_Core)
dpg.bind_theme(global_theme)

def resize_windows_handler():
    w = max(dpg.get_viewport_width() - 16, 400)
    h = max(dpg.get_viewport_height() - 39, 300)
    for screen in ["landing_hub_window", "workbench_window"]:
        if dpg.does_item_exist(screen):
            dpg.configure_item(screen, width=w, height=h, pos=[0, 0])

# ==============================================================================
# SCREEN LAYER 1: LANDING HUB
# ==============================================================================
with dpg.window(tag="landing_hub_window", no_move=True, no_resize=True, no_title_bar=True, show=True):
    dpg.add_text("Backtesting Engine By Vivek Venigalla", color=[100, 200, 255])
    dpg.add_separator()
    dpg.add_spacer(height=10)
    with dpg.group(horizontal=True):
        with dpg.child_window(width=320, height=-1):
            dpg.add_button(label="+ Create New Simulation Batch", width=-1, height=50, callback=lambda: route_to_view("workbench_window"))
        with dpg.child_window(width=-1, height=-1):
            dpg.add_text("Historical Batches Directory (From batchConfig/)", color=[140, 140, 140])
            dpg.add_separator()
            if not core.state["historical_batches"]:
                dpg.add_text("No historical configurations found.", color=[255, 165, 0])
            else:
                for batch in core.state["historical_batches"]:
                    dpg.add_text(f"📂 {batch['batch_id']}.json (Modified: {batch['timestamp']})")

# ==============================================================================
# SCREEN LAYER 2: WORKBENCH (With Step 3 Left Pane Population)
# ==============================================================================
with dpg.window(tag="workbench_window", no_move=True, no_resize=True, no_title_bar=True, show=False):
    with dpg.group(horizontal=True):
        dpg.add_button(label="Back to Hub", callback=lambda: route_to_view("landing_hub_window"))
        dpg.add_text("WORKBENCH PIPELINE MANAGEMENT", color=[100, 200, 255])
    dpg.add_separator()
    dpg.add_spacer(height=10)
    
    with dpg.group(horizontal=True):
        # LEFT PANE: Metadata and Entity Selection Registers
        with dpg.child_window(width=360, height=-1):
            dpg.add_input_text(label="Batch ID", tag="ui_batch_id", default_value=active_batch["simulation_metadata"]["batch_id"])
            dpg.add_input_text(label="Notes", tag="ui_batch_notes", default_value=active_batch["simulation_metadata"]["notes"])
            dpg.add_spacer(height=10)
            
            dpg.add_text("Global Shared Infrastructure Registers", color=[100, 255, 100])
            dpg.add_separator()
            
            # Accounts Collapsible Container
            with dpg.collapsing_header(label="Available Accounts"):
                dpg.add_button(label="+ Create New Account Blueprint", width=-1, callback=spawn_create_account_modal)
                dpg.add_spacer(height=5)
                with dpg.group(tag="reg_accounts_container"): pass
                
            # Brokers Collapsible Container
            with dpg.collapsing_header(label="Available Brokers"):
                dpg.add_button(label="+ Create New Broker Blueprint", width=-1, callback=spawn_create_broker_modal)
                dpg.add_spacer(height=5)
                with dpg.group(tag="reg_brokers_container"): pass
                
            # Data Feeds Collapsible Container
            with dpg.collapsing_header(label="Available Data Feeds"):
                dpg.add_button(label="+ Create New Feed Blueprint", width=-1, callback=spawn_create_feed_modal)
                dpg.add_spacer(height=5)
                with dpg.group(tag="reg_feeds_container"): pass

        # RIGHT PANE: Canvas Placeholder for Step 4 & 5
        with dpg.child_window(width=-1, height=-1):
            dpg.add_text("Active Batch Workstation Canvas (Step 4 & 5)", color=[140, 140, 140])

# ==============================================================================
# INITIALIZATION & EXECUTION
# ==============================================================================
dpg.create_viewport(title='Backtesting Engine', width=1300, height=840)
dpg.setup_dearpygui()
dpg.show_viewport()

dpg.set_viewport_resize_callback(resize_windows_handler)
resize_windows_handler()

# Render side-pane contents from core.state safely
refresh_registry_sidepane()

dpg.start_dearpygui()
dpg.destroy_context()