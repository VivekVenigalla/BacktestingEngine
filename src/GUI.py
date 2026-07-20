import dearpygui.dearpygui as dpg
import json
from core import state, save_batch_config  # Import backend variables & functions

dpg.create_context()

# --- Active Batch State Schema ---
active_batch = {
    "simulation_metadata": {"batch_id": "batch_123", "notes": ""},
    "account": [],
    "broker": [],
    "data_feeds": [],
    "simulations": []
}

# --- Dynamic UI Refresh Hooks ---
def refresh_ui_state():
    """Redraws the batch tables and lists based on the active_batch dictionary."""
    
    # 1. Refresh Accounts safely (Avoid adding hardcoded tags to row children)
    if dpg.does_item_exist("batch_accounts_group"):
        dpg.delete_item("batch_accounts_group", children_only=True)
        for idx, acct in enumerate(active_batch["account"]):
            with dpg.group(horizontal=True, parent="batch_accounts_group"):
                dpg.add_text(f"💳 {acct['id']} (${acct['initial_balance']})")
                dpg.add_button(label="✏️", user_data=idx, callback=spawn_edit_account_modal)
            
    # 2. Refresh Brokers safely
    if dpg.does_item_exist("batch_brokers_group"):
        dpg.delete_item("batch_brokers_group", children_only=True)
        for idx, brk in enumerate(active_batch["broker"]):
            with dpg.group(horizontal=True, parent="batch_brokers_group"):
                dpg.add_text(f"🏦 {brk['id']} (Acct: {brk.get('account_link', 'None')})")
                dpg.add_button(label="✏️", user_data=idx, callback=spawn_edit_broker_modal)

    # 3. Refresh Feeds safely
    if dpg.does_item_exist("batch_feeds_group"):
        dpg.delete_item("batch_feeds_group", children_only=True)
        for idx, fd in enumerate(active_batch["data_feeds"]):
            with dpg.group(horizontal=True, parent="batch_feeds_group"):
                dpg.add_text(f"📈 {fd['id']} ({fd['timeframe']})")
                dpg.add_button(label="✏️", user_data=idx, callback=spawn_edit_feed_modal)

    # 4. Bulletproof Rebuild of the Simulation Pipeline Table
    if dpg.does_item_exist("ui_table_wrapper"):
        dpg.delete_item("ui_table_wrapper", children_only=True)
        
    with dpg.table(parent="ui_table_wrapper", header_row=True, 
                   borders_innerH=True, borders_outerH=True, 
                   borders_innerV=True, borders_outerV=True, resizable=True):
        
        dpg.add_table_column(label="Sim ID")
        dpg.add_table_column(label="Strategy & Params")
        dpg.add_table_column(label="Acct Link")
        dpg.add_table_column(label="Broker Link")
        dpg.add_table_column(label="Feeds")
        dpg.add_table_column(label="Actions", width_fixed=True)
        
        for idx, sim in enumerate(active_batch["simulations"]):
            with dpg.table_row():
                dpg.add_text(sim["id"])
                dpg.add_text(f"{sim['strategy']} {sim['parameters']}")
                dpg.add_text(sim.get("account_link", "None"))
                dpg.add_text(sim.get("broker_link", "None"))
                dpg.add_text(", ".join(sim.get("feeds", [])))
                dpg.add_button(label="✏️ Edit", user_data=idx, callback=spawn_edit_sim_modal)

# --- Add from Global Registers ---
def add_entity_to_batch(sender, app_data, user_data):
    entity_type, entity_data = user_data
    if entity_type == "ACCOUNT":
        active_batch["account"].append(dict(entity_data))
    elif entity_type == "BROKER":
        active_batch["broker"].append(dict(entity_data))
    elif entity_type == "FEED":
        active_batch["data_feeds"].append(dict(entity_data))
    refresh_ui_state()

# --- Edit Modals ---
def close_modal(modal_tag):
    if dpg.does_item_exist(modal_tag): dpg.delete_item(modal_tag)

def spawn_edit_account_modal(sender, app_data, user_data):
    idx = user_data
    acct = active_batch["account"][idx]
    close_modal("modal_edit_acct")
    
    with dpg.window(label=f"Edit Batch Account: {acct['id']}", tag="modal_edit_acct", modal=True, width=350, height=200):
        dpg.add_input_text(label="ID", tag="e_acct_id", default_value=acct["id"])
        dpg.add_input_float(label="Balance", tag="e_acct_bal", default_value=acct["initial_balance"])
        dpg.add_checkbox(label="Reset", tag="e_acct_reset", default_value=acct["reset"])
        
        def save():
            acct["id"] = dpg.get_value("e_acct_id")
            acct["initial_balance"] = dpg.get_value("e_acct_bal")
            acct["reset"] = dpg.get_value("e_acct_reset")
            refresh_ui_state()
            close_modal("modal_edit_acct")
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Changes", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_edit_acct"))

def spawn_edit_broker_modal(sender, app_data, user_data):
    idx = user_data
    brk = active_batch["broker"][idx]
    close_modal("modal_edit_broker")
    
    with dpg.window(label=f"Edit Batch Broker: {brk['id']}", tag="modal_edit_broker", modal=True, width=400, height=250):
        dpg.add_input_text(label="ID", tag="e_brk_id", default_value=brk["id"])
        dpg.add_input_float(label="Commission Rate", tag="e_brk_comm", default_value=brk["commission_rate"])
        dpg.add_input_float(label="Slippage Rate", tag="e_brk_slip", default_value=brk["slippage_rate"])
        dpg.add_input_text(label="Account Link", tag="e_brk_link", default_value=brk.get("account_link", ""))
        dpg.add_checkbox(label="Reset", tag="e_brk_reset", default_value=brk["reset"])
        
        def save():
            brk["id"] = dpg.get_value("e_brk_id")
            brk["commission_rate"] = dpg.get_value("e_brk_comm")
            brk["slippage_rate"] = dpg.get_value("e_brk_slip")
            brk["account_link"] = dpg.get_value("e_brk_link")
            brk["reset"] = dpg.get_value("e_brk_reset")
            refresh_ui_state()
            close_modal("modal_edit_broker")
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Changes", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_edit_broker"))

def spawn_edit_feed_modal(sender, app_data, user_data):
    idx = user_data
    fd = active_batch["data_feeds"][idx]
    close_modal("modal_edit_feed")
    
    with dpg.window(label=f"Edit Data Feed: {fd['id']}", tag="modal_edit_feed", modal=True, width=400, height=250):
        dpg.add_input_text(label="ID", tag="e_fd_id", default_value=fd["id"])
        dpg.add_input_text(label="Ticker", tag="e_fd_tick", default_value=fd["ticker"])
        dpg.add_input_text(label="Timeframe", tag="e_fd_tf", default_value=fd["timeframe"])
        dpg.add_input_int(label="CAGR Length", tag="e_fd_cagr", default_value=fd["cagr_length"])
        dpg.add_input_text(label="CSV Filepath", tag="e_fd_csv", default_value=fd["csv_filepath"])
        
        def save():
            fd["id"] = dpg.get_value("e_fd_id")
            fd["ticker"] = dpg.get_value("e_fd_tick")
            fd["timeframe"] = dpg.get_value("e_fd_tf")
            fd["cagr_length"] = dpg.get_value("e_fd_cagr")
            fd["csv_filepath"] = dpg.get_value("e_fd_csv")
            refresh_ui_state()
            close_modal("modal_edit_feed")
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Changes", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_edit_feed"))

def spawn_edit_sim_modal(sender, app_data, user_data):
    idx = user_data
    sim = active_batch["simulations"][idx]
    close_modal("modal_edit_sim")
    
    with dpg.window(label=f"Edit Simulation: {sim['id']}", tag="modal_edit_sim", modal=True, width=450, height=350):
        dpg.add_input_text(label="Sim ID", tag="e_sim_id", default_value=sim["id"])
        dpg.add_input_text(label="Strategy Class", tag="e_sim_strat", default_value=sim["strategy"])
        
        param_str = json.dumps(sim["parameters"])
        dpg.add_input_text(label="Params (JSON)", tag="e_sim_params", default_value=param_str, width=-1)
        
        dpg.add_input_text(label="Feeds (Comma sep)", tag="e_sim_feeds", default_value=",".join(sim["feeds"]))
        dpg.add_input_text(label="Account Link", tag="e_sim_acct", default_value=sim["account_link"])
        dpg.add_input_text(label="Broker Link", tag="e_sim_brk", default_value=sim["broker_link"])
        dpg.add_checkbox(label="Run by Default", tag="e_sim_run", default_value=sim["run_all_by_default"])
        
        def save():
            sim["id"] = dpg.get_value("e_sim_id")
            sim["strategy"] = dpg.get_value("e_sim_strat")
            try: sim["parameters"] = json.loads(dpg.get_value("e_sim_params"))
            except: pass
            sim["feeds"] = [x.strip() for x in dpg.get_value("e_sim_feeds").split(",")]
            sim["account_link"] = dpg.get_value("e_sim_acct")
            sim["broker_link"] = dpg.get_value("e_sim_brk")
            sim["run_all_by_default"] = dpg.get_value("e_sim_run")
            refresh_ui_state()
            close_modal("modal_edit_sim")
            
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Changes", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_edit_sim"))

def add_new_simulation():
    new_sim = {
        "id": f"new_sim_{len(active_batch['simulations'])+1}",
        "strategy": "sma",
        "parameters": {"fast_period": 50, "slow_period": 200},
        "feeds": [],
        "account_link": "",
        "broker_link": "",
        "run_all_by_default": True
    }
    active_batch["simulations"].append(new_sim)
    refresh_ui_state()

def save_active_batch():
    active_batch["simulation_metadata"]["batch_id"] = dpg.get_value("ui_batch_id")
    active_batch["simulation_metadata"]["notes"] = dpg.get_value("ui_batch_notes")
    save_batch_config(active_batch)

def route_to_view(target_window_tag):
    for view in ["landing_hub_window", "workbench_window"]:
        dpg.configure_item(view, show=(view == target_window_tag))

# --- Application Layout Hierarchy ---
with dpg.window(tag="landing_hub_window", no_move=True, no_resize=True, no_title_bar=True, pos=[0,0]):
    dpg.add_text("QUANTITATIVE RUN ARCHIVE LANDING STATION", color=[100, 200, 255])
    dpg.add_separator()
    with dpg.group(horizontal=True):
        dpg.add_button(label="➕ Create New Batch", width=300, height=50, callback=lambda: route_to_view("workbench_window"))
        with dpg.child_window():
            dpg.add_text("Historical Batches (From core.state)")
            for b in state["historical_batches"]:
                dpg.add_text(f"📂 {b['batch_id']} (Last Modified: {b['timestamp']})")

with dpg.window(tag="workbench_window", no_move=True, no_resize=True, no_title_bar=True, show=False, pos=[0,0]):
    with dpg.group(horizontal=True):
        dpg.add_button(label="⬅ Back", callback=lambda: route_to_view("landing_hub_window"))
        dpg.add_text("WORKBENCH PIPELINE MANAGEMENT", color=[100, 200, 255])
    dpg.add_separator()
    
    with dpg.group(horizontal=True):
        # LEFT PANE: Metadata and Entry Selection Registers
        with dpg.child_window(width=350, height=-1):
            dpg.add_input_text(label="Batch ID", tag="ui_batch_id", default_value=active_batch["simulation_metadata"]["batch_id"])
            dpg.add_input_text(label="Notes", tag="ui_batch_notes", default_value=active_batch["simulation_metadata"]["notes"])
            dpg.add_spacer(height=10)
            
            dpg.add_text("Global Registered Entities", color=[100, 255, 100])
            dpg.add_separator()
            with dpg.collapsing_header(label="Available Accounts"):
                for acct in state["registered_accounts"]:
                    dpg.add_button(label=f"+ {acct['id']}", user_data=("ACCOUNT", acct), callback=add_entity_to_batch)
            with dpg.collapsing_header(label="Available Brokers"):
                for brk in state["registered_brokers"]:
                    dpg.add_button(label=f"+ {brk['id']}", user_data=("BROKER", brk), callback=add_entity_to_batch)
            with dpg.collapsing_header(label="Available Data Feeds"):
                for fd in state["registered_feeds"]:
                    dpg.add_button(label=f"+ {fd['id']}", user_data=("FEED", fd), callback=add_entity_to_batch)
            
            dpg.add_spacer(height=20)
            dpg.add_button(label="💾 COMPILE & SAVE BATCH JSON", width=-1, height=45, callback=save_active_batch)

        # RIGHT PANE: Batch Editor Containers
        with dpg.child_window(width=-1, height=-1):
            with dpg.group(horizontal=True):
                with dpg.child_window(width=200, height=180):
                    dpg.add_text("Batch Accounts")
                    dpg.add_separator()
                    with dpg.group(tag="batch_accounts_group"): pass
                with dpg.child_window(width=250, height=180):
                    dpg.add_text("Batch Brokers")
                    dpg.add_separator()
                    with dpg.group(tag="batch_brokers_group"): pass
                with dpg.child_window(width=-1, height=180):
                    dpg.add_text("Batch Data Feeds")
                    dpg.add_separator()
                    with dpg.group(tag="batch_feeds_group"): pass
                    
            dpg.add_spacer(height=10)
            with dpg.group(horizontal=True):
                dpg.add_text("Batch Simulations Pipeline")
                dpg.add_button(label="➕ Add New Simulation", callback=add_new_simulation)
                
            # Dynamic table wrapper container
            with dpg.group(tag="ui_table_wrapper"): pass

# --- Global Style & Window Resizing Handler ---
with dpg.theme() as global_theme:
    with dpg.theme_component(dpg.mvAll):
        dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 12, 12, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_FramePadding, 6, 6, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing, 8, 8, category=dpg.mvThemeCat_Core)
dpg.bind_theme(global_theme)

def resize_windows_handler():
    w, h = dpg.get_viewport_width() - 16, dpg.get_viewport_height() - 39
    for item in ["landing_hub_window", "workbench_window"]:
        if dpg.does_item_exist(item): dpg.configure_item(item, width=w, height=h, pos=[0, 0])

dpg.create_viewport(title='Algorithmic Simulation Framework', width=1300, height=840)
dpg.setup_dearpygui()
dpg.show_viewport()
dpg.set_viewport_resize_callback(resize_windows_handler)
resize_windows_handler()

# Execute initial render safely now that structural tags are securely established on the layout stack
refresh_ui_state() 

dpg.start_dearpygui()
dpg.destroy_context()