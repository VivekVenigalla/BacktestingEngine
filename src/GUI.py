import dearpygui.dearpygui as dpg

dpg.create_context()

# =========================================================================
# 💾 STATE CACHE / REGISTERS
# =========================================================================
registered_accounts = ["Acct_Retail_1", "Acct_Institutional_Main"]
registered_brokers = ["Broker_LowSlippage_Std", "Broker_ZeroComm_VIP"]
registered_feeds = ["AAPL_Daily_CSV", "MSFT_Daily_CSV"]

historical_batches_mock = [
    {"batch_id": "Batch_2026_07_15", "timestamp": "2026-07-15 14:22"},
    {"batch_id": "Batch_2026_07_18", "timestamp": "2026-07-18 09:05"}
]

# =========================================================================
# 🎛️ MODAL POPUP WINDOW GENERATORS (Auto-Centered)
# =========================================================================
def spawn_create_account_modal():
    if dpg.does_item_exist("modal_create_account"): dpg.delete_item("modal_create_account")
    
    # Simple manual centering math base calculations
    v_w = dpg.get_viewport_width()
    v_h = dpg.get_viewport_height()
    
    with dpg.window(label="Instantiate New Account", tag="modal_create_account", modal=True, 
                    width=350, height=200, no_resize=True, pos=[int(v_w/2 - 175), int(v_h/2 - 100)]):
        dpg.add_input_text(label="Account ID", tag="modal_acct_id", default_value="Acct_New_Custom")
        dpg.add_input_float(label="Initial Balance", tag="modal_acct_bal", default_value=50000.0, format="$%.2f")
        dpg.add_spacer(height=10)
        
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", width=120, callback=submit_account_callback)
            dpg.add_button(label="Cancel", width=120, callback=lambda: dpg.delete_item("modal_create_account"))

def spawn_create_broker_modal():
    if dpg.does_item_exist("modal_create_broker"): dpg.delete_item("modal_create_broker")
    v_w, v_h = dpg.get_viewport_width(), dpg.get_viewport_height()
    
    with dpg.window(label="Configure Broker Gateway", tag="modal_create_broker", modal=True, 
                    width=350, height=220, no_resize=True, pos=[int(v_w/2 - 175), int(v_h/2 - 110)]):
        dpg.add_input_text(label="Broker ID", tag="modal_broker_id", default_value="Broker_Custom_Direct")
        dpg.add_input_float(label="Commission Rate", tag="modal_broker_comm", default_value=0.0005, format="%.5f")
        dpg.add_input_float(label="Slippage Rate", tag="modal_broker_slip", default_value=0.0002, format="%.5f")
        dpg.add_spacer(height=10)
        
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", width=120, callback=submit_broker_callback)
            dpg.add_button(label="Cancel", width=120, callback=lambda: dpg.delete_item("modal_create_broker"))

def spawn_create_feed_modal():
    if dpg.does_item_exist("modal_create_feed"): dpg.delete_item("modal_create_feed")
    v_w, v_h = dpg.get_viewport_width(), dpg.get_viewport_height()
    
    with dpg.window(label="Register Historical Tape Feed", tag="modal_create_feed", modal=True, 
                    width=400, height=220, no_resize=True, pos=[int(v_w/2 - 200), int(v_h/2 - 110)]):
        dpg.add_input_text(label="Data Feed ID", tag="modal_feed_id", default_value="TSLA_Daily_CSV")
        dpg.add_input_text(label="Asset Ticker", tag="modal_feed_ticker", default_value="TSLA")
        dpg.add_input_text(label="CSV Filepath", tag="modal_feed_path", default_value="../data/tsla_raw.csv")
        dpg.add_spacer(height=10)
        
        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", width=120, callback=submit_feed_callback)
            dpg.add_button(label="Cancel", width=120, callback=lambda: dpg.delete_item("modal_create_feed"))

# =========================================================================
# 📥 CALLBACKS & PIPELINE ACTIONS
# =========================================================================
def submit_account_callback():
    acct_id = dpg.get_value("modal_acct_id")
    registered_accounts.append(acct_id)
    lbl = dpg.add_selectable(label=f"💳 {acct_id}", parent="account_list_container")
    with dpg.drag_payload(parent=lbl, payload_type="ACCOUNT_PAYLOAD", drag_data=acct_id): dpg.add_text(f"Moving: {acct_id}")
    dpg.delete_item("modal_create_account")

def submit_broker_callback():
    broker_id = dpg.get_value("modal_broker_id")
    registered_brokers.append(broker_id)
    lbl = dpg.add_selectable(label=f"💼 {broker_id}", parent="broker_list_container")
    with dpg.drag_payload(parent=lbl, payload_type="BROKER_PAYLOAD", drag_data=broker_id): dpg.add_text(f"Moving: {broker_id}")
    dpg.delete_item("modal_create_broker")

def submit_feed_callback():
    feed_id = dpg.get_value("modal_feed_id")
    registered_feeds.append(feed_id)
    lbl = dpg.add_selectable(label=f"📈 {feed_id}", parent="feed_list_container")
    with dpg.drag_payload(parent=lbl, payload_type="FEED_PAYLOAD", drag_data=feed_id): dpg.add_text(f"Moving: {feed_id}")
    dpg.delete_item("modal_create_feed")

def add_strategy_to_batch():
    sim_id = dpg.get_value("ui_workbench_sim_id")
    strat = dpg.get_value("ui_workbench_strat")
    acct = dpg.get_value("ui_workbench_acct")
    broker = dpg.get_value("ui_workbench_broker")
    feeds = dpg.get_value("ui_workbench_feeds")
    
    with dpg.table_row(parent="ui_workbench_batch_table"):
        dpg.add_text(sim_id)
        dpg.add_text(strat)
        dpg.add_text(acct)
        dpg.add_text(broker)
        dpg.add_text(feeds)

def edit_historical_batch_callback(sender, app_data, user_data):
    # Placeholders to set config values using historical state folder records
    dpg.set_value("ui_workbench_batch_id", user_data["batch_id"])
    print(f"Hydrating workbench configuration variables from file maps: output/{user_data['batch_id']}/simConfig.json")
    route_to_view("workbench_window")

def load_historical_batch_callback(sender, app_data, user_data):
    print(f"Loading historical graphics states from target output analytics logs directory folder: output/{user_data['batch_id']}/")
    route_to_view("results_view_window")

def run_compiled_batch_pipeline():
    route_to_view("results_view_window")

# =========================================================================
# 🗺️ ROUTING SCENE MANAGER
# =========================================================================
def route_to_view(target_window_tag):
    views = ["landing_hub_window", "workbench_window", "results_view_window"]
    for view in views:
        dpg.configure_item(view, show=(view == target_window_tag))

# =========================================================================
# 🏠 SCREEN 1: LANDING HUB (HISTORICAL LOOKUPS VS CREATION ROUTES)
# =========================================================================
with dpg.window(tag="landing_hub_window", no_move=True, no_resize=True, no_title_bar=True, pos=[0, 0]):
    dpg.add_text("QUANTITATIVE RUN ARCHIVE LANDING STATION", color=[100, 200, 255])
    dpg.add_separator()
    
    with dpg.group(horizontal=True):
        with dpg.child_window(width=400, height=-1):
            dpg.add_text("Configure Fresh Sequence Run Setup", color=[255, 200, 100])
            dpg.add_spacer(height=10)
            dpg.add_button(label="➕ Create New Simulation Batch", width=-1, height=50, 
                           callback=lambda: route_to_view("workbench_window"))
            
        with dpg.child_window(width=-1, height=-1):
            dpg.add_text("Scan and Reload Previously Executed Batch Subfolders", color=[100, 255, 100])
            dpg.add_separator()
            
            with dpg.table(header_row=True, borders_innerH=True, borders_outerH=True, borders_innerV=True, borders_outerV=True):
                dpg.add_table_column(label="Batch Folder Instance ID")
                dpg.add_table_column(label="Recorded Runtime Timestamp")
                dpg.add_table_column(label="Operations Actions Hook", width_fixed=True)
                
                for batch in historical_batches_mock:
                    with dpg.table_row():
                        dpg.add_text(batch["batch_id"])
                        dpg.add_text(batch["timestamp"])
                        with dpg.group(horizontal=True):
                            dpg.add_button(label="📂 View Data", user_data=batch, callback=load_historical_batch_callback)
                            dpg.add_button(label="✏️ Edit Config", user_data=batch, callback=edit_historical_batch_callback)

# =========================================================================
# 🛠️ SCREEN 2: BATCH MANAGEMENT DASHBOARD & MODAL INSTANTIATOR WORKBENCH
# =========================================================================
with dpg.window(tag="workbench_window", no_move=True, no_resize=True, no_title_bar=True, show=False, pos=[0, 0]):
    with dpg.group(horizontal=True):
        dpg.add_button(label="⬅ Back to Hub Main Page", callback=lambda: route_to_view("landing_hub_window"))
        dpg.add_text("DISTRIBUTED INFRASTRUCTURE WORKBENCH MANAGEMENT ENVIRONMENT", color=[100, 200, 255])
    dpg.add_separator()
    
    with dpg.group(horizontal=True):
        with dpg.child_window(width=420, height=-1):
            dpg.add_text("Batch Properties", color=[255, 200, 100])
            dpg.add_input_text(label="Batch ID (Folder)", tag="ui_workbench_batch_id", default_value="Batch_Run_Alpha")
            dpg.add_separator()
            
            dpg.add_text("Strategy Specification Layer Mapping Form")
            dpg.add_input_text(label="Simulation Run ID", tag="ui_workbench_sim_id", default_value="Sim_Instance_1")
            dpg.add_combo(label="Strategy Class", tag="ui_workbench_strat", items=["Donchian Channel", "SMA Cross"], default_value="Donchian Channel")
            
            dpg.add_spacer(height=5)
            dpg.add_input_text(label="Target Account ID", tag="ui_workbench_acct", readonly=True, drop_callback=lambda s, a: dpg.set_value("ui_workbench_acct", a), payload_type="ACCOUNT_PAYLOAD")
            dpg.add_input_text(label="Gateway Broker ID", tag="ui_workbench_broker", readonly=True, drop_callback=lambda s, a: dpg.set_value("ui_workbench_broker", a), payload_type="BROKER_PAYLOAD")
            dpg.add_input_text(label="Subscribed Tapes", tag="ui_workbench_feeds", readonly=True, drop_callback=lambda s, a: dpg.set_value("ui_workbench_feeds", a), payload_type="FEED_PAYLOAD")
            
            dpg.add_spacer(height=10)
            dpg.add_button(label="➕ Add Configuration Snapshot to Active Batch Queue", width=-1, height=35, callback=add_strategy_to_batch)
            dpg.add_spacer(height=15)
            dpg.add_button(label="⚡ RUN COMPILED BATCH PIPELINE SUBPROCESSES", width=-1, height=45, callback=run_compiled_batch_pipeline)

        with dpg.child_window(width=-1, height=-1):
            with dpg.group(horizontal=True):
                dpg.add_text("Infrastructure Resource Base Objects Register", color=[100, 255, 100])
                dpg.add_spacer(width=20)
                dpg.add_button(label="+ Account", callback=spawn_create_account_modal)
                dpg.add_button(label="+ Broker", callback=spawn_create_broker_modal)
                dpg.add_button(label="+ Feed Tape", callback=spawn_create_feed_modal)
                
            dpg.add_separator()
            
            with dpg.group(horizontal=True):
                with dpg.child_window(width=250, height=180):
                    dpg.add_text("Accounts Hub List")
                    with dpg.group(tag="account_list_container"):
                        for a in registered_accounts:
                            lbl = dpg.add_selectable(label=f"💳 {a}")
                            with dpg.drag_payload(parent=lbl, payload_type="ACCOUNT_PAYLOAD", drag_data=a): dpg.add_text(f"Moving {a}")
                            
                with dpg.child_window(width=250, height=180):
                    dpg.add_text("Brokers Hub List")
                    with dpg.group(tag="broker_list_container"):
                        for b in registered_brokers:
                            lbl = dpg.add_selectable(label=f"💼 {b}")
                            with dpg.drag_payload(parent=lbl, payload_type="BROKER_PAYLOAD", drag_data=b): dpg.add_text(f"Moving {b}")
                            
                with dpg.child_window(width=250, height=180):
                    dpg.add_text("Data Tapes Hub List")
                    with dpg.group(tag="feed_list_container"):
                        for f in registered_feeds:
                            lbl = dpg.add_selectable(label=f"📈 {f}")
                            with dpg.drag_payload(parent=lbl, payload_type="FEED_PAYLOAD", drag_data=f): dpg.add_text(f"Moving {f}")

            dpg.add_spacer(height=15)
            dpg.add_text("Active Target Simulation Execution Pipeline Queue Batch", color=[100, 200, 255])
            
            with dpg.table(header_row=True, tag="ui_workbench_batch_table", borders_innerH=True, borders_outerH=True, borders_innerV=True, borders_outerV=True, resizable=True):
                dpg.add_table_column(label="Sim ID")
                dpg.add_table_column(label="Strategy Type")
                dpg.add_table_column(label="Account Target")
                dpg.add_table_column(label="Broker Gateway Link")
                dpg.add_table_column(label="Data Subscriptions")

# =========================================================================
# 📊 SCREEN 3: ANALYTICS RUN METRICS PANEL (Horizontal Splitting Design)
# =========================================================================
with dpg.window(tag="results_view_window", no_move=True, no_resize=True, no_title_bar=True, show=False, pos=[0, 0]):
    with dpg.group(horizontal=True):
        dpg.add_button(label="⬅ Back to Workbench", callback=lambda: route_to_view("workbench_window"))
        dpg.add_text("BATCH ANALYTICS VERIFICATION INSIGHT MONITOR", color=[100, 200, 255])
    dpg.add_separator()
    
    with dpg.group(horizontal=True):
        # LEFT SIDEBAR SUB-PANEL: Performance Statistics Grid Section
        with dpg.child_window(width=320, height=-1):
            dpg.add_text("Core Performance Statistics", color=[255, 200, 100])
            dpg.add_separator()
            
            # Form-structured dashboard cards to frame output arrays
            dpg.add_input_text(label="CAGR", default_value="18.54 %", readonly=True)
            dpg.add_input_text(label="Total Return", default_value="142.30 %", readonly=True)
            dpg.add_input_text(label="Max Drawdown", default_value="-11.24 %", readonly=True)
            dpg.add_input_text(label="Sharpe Ratio", default_value="1.82", readonly=True)
            dpg.add_input_text(label="Sortino Ratio", default_value="2.15", readonly=True)
            dpg.add_input_text(label="Win Rate Factor", default_value="58.4%", readonly=True)
            dpg.add_input_text(label="Profit Factor", default_value="1.45", readonly=True)
            
        # RIGHT SIDEBAR SUB-PANEL: Takes remaining canvas window real estate for plots
        with dpg.child_window(width=-1, height=-1):
            dpg.add_text("Visual Performance Track Plots Workspace Canvas Canvas Stack", color=[100, 255, 100])
            dpg.add_separator()
            
            with dpg.child_window(height=300, label="Equity Plot Box Container Area"):
                dpg.add_text("[Equity Time-Series Interactive Performance Curve Graph Plot Node Area]")
                
            dpg.add_spacer(height=10)
            
            with dpg.child_window(height=-1, label="Drawdown Plot Box Container Area"):
                dpg.add_text("[Drawdown Time-Series Trailing Dev Deviation Area Graph Plot Node Area]")

# =========================================================================
# 📐 GLOBAL THEME PAD FIXES & RESIZING EVENT HANDLERS
# =========================================================================
# System Theme overrides to kill empty black edge offsets completely
with dpg.theme() as global_theme:
    with dpg.theme_component(dpg.mvAll):
        dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 0, 0, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_FramePadding, 4, 4, category=dpg.mvThemeCat_Core)

dpg.bind_theme(global_theme)

def resize_windows_handler():
    # Use exact OS frame values to flush the background view matrix lines out
    w = dpg.get_viewport_width()
    h = dpg.get_viewport_height()
    
    # Adjust for operating system title titlebars offsets automatically
    adjusted_w = w - 16
    adjusted_h = h - 39
    
    for item in ["landing_hub_window", "workbench_window", "results_view_window"]:
        if dpg.does_item_exist(item):
            dpg.configure_item(item, width=adjusted_w, height=adjusted_h)

dpg.create_viewport(title='Enterprise Algorithmic Simulation Workspace Framework Ecosystem', width=1300, height=820)
dpg.setup_dearpygui()
dpg.show_viewport()
dpg.set_viewport_resize_callback(resize_windows_handler)
resize_windows_handler()

dpg.start_dearpygui()
dpg.destroy_context()