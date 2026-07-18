import dearpygui.dearpygui as dpg

#NOTE THIS IS ALL A SKELETON FOR THE FUNCTIONS THE TAKE PLACE IN

dpg.create_context()

# --- Global Application State Cache (Placeholders) ---
global_accounts = ["Acct_Retail_1", "Acct_Institutional_Main"]
global_brokers = ["Broker_LowSlippage_Std", "Broker_ZeroComm_VIP"]
global_feeds = ["AAPL_Daily_CSV", "MSFT_Daily_CSV", "SPY_Intraday_1Min"]

active_batch_simulations = [
    {"sim_id": "Sim_Donchian_AAPL", "strat": "Donchian Channel", "acct": "Acct_Retail_1", "broker": "Broker_LowSlippage_Std", "feeds": ["AAPL_Daily_CSV"]},
    {"sim_id": "Sim_SMACross_MSFT", "strat": "SMA Cross", "acct": "Acct_Retail_1", "broker": "Broker_ZeroComm_VIP", "feeds": ["MSFT_Daily_CSV"]}
]

# --- Drag & Drop Payload Drop Callbacks ---
def on_account_drop(sender, app_data, user_data):
    # app_data is the drag_data passed from the source drag_payload container
    dpg.set_value("strat_input_acct_link", app_data)

def on_broker_drop(sender, app_data, user_data):
    dpg.set_value("strat_input_broker_link", app_data)

def on_feed_drop(sender, app_data, user_data):
    current_val = dpg.get_value("strat_input_feeds_link")
    new_val = f"{current_val}, {app_data}" if current_val else app_data
    dpg.set_value("strat_input_feeds_link", new_val)




# --- UI Pipeline Action Handlers ---
def add_sim_to_batch_callback():
    sim_id = dpg.get_value("strat_input_id")
    strat_type = dpg.get_value("strat_input_type")
    acct = dpg.get_value("strat_input_acct_link")
    broker = dpg.get_value("strat_input_broker_link")
    feeds_str = dpg.get_value("strat_input_feeds_link")
    
    print(f"Adding to Batch Queue -> Sim: {sim_id} using {strat_type}")
    


    with dpg.table_row(parent="batch_queue_table"):
        dpg.add_text(sim_id)
        dpg.add_text(strat_type)
        dpg.add_text(acct)
        dpg.add_text(broker)
        dpg.add_text(feeds_str)

def run_batch_pipeline_callback():
    print("Serializing batch payload array configurations to simConfig.json...")
    show_results_screen()

def show_home_screen():
    dpg.configure_item("home_screen_window", show=True)
    dpg.configure_item("results_window", show=False)

def show_results_screen():
    dpg.configure_item("home_screen_window", show=False)
    dpg.configure_item("results_window", show=True)


# =========================================================================
# 🏠 VIEW 1: CENTRAL COMMAND & DRAG-AND-DROP SIM WORKSPACE
# =========================================================================
with dpg.window(label="Home Screen", tag="home_screen_window", no_move=True, no_resize=True, no_title_bar=True):
    dpg.add_text("QUANTITATIVE BACKTESTING WORKSPACE PLATFORM SYSTEM v2.0", color=[100, 200, 255])
    dpg.add_separator()
    
    with dpg.group(horizontal=True):
        
        # --- LEFT PANEL: Strategy Instantiation Builder Sandbox Area ---
        with dpg.child_window(width=420, height=670, label="Strategy Workbench"):
            dpg.add_text("1. Build / Configure Strategy Linkages", color=[255, 200, 100])
            dpg.add_spacer(height=5)
            
            dpg.add_input_text(label="Simulation ID", tag="strat_input_id", default_value="Sim_Donchian_Live")
            dpg.add_combo(label="Strategy Class", tag="strat_input_type", items=["Donchian Channel", "SMA Cross", "Bollinger Bands"], default_value="Donchian Channel")
            
            dpg.add_spacer(height=10)
            dpg.add_text("Drag Components From Right Panel Into These Target Fields:", color=[150, 150, 150])
            dpg.add_separator()
            
            # --- DROP TARGET FIELDS (Using drop_callback directly on the widget) ---
            dpg.add_input_text(label="Linked Account ID", tag="strat_input_acct_link", default_value="", readonly=True,
                               drop_callback=on_account_drop, payload_type="ACCOUNT_TYPE_PAYLOAD")
                
            dpg.add_input_text(label="Linked Broker ID", tag="strat_input_broker_link", default_value="", readonly=True,
                               drop_callback=on_broker_drop, payload_type="BROKER_TYPE_PAYLOAD")
                
            dpg.add_input_text(label="Linked Data Feeds", tag="strat_input_feeds_link", default_value="", readonly=True,
                               drop_callback=on_feed_drop, payload_type="FEED_TYPE_PAYLOAD")

            dpg.add_spacer(height=10)
            dpg.add_text("Hyperparameters Configuration Tuning")
            dpg.add_input_int(label="Window Lookback", default_value=20)
            dpg.add_input_float(label="Risk Allocation Factor", default_value=0.95, format="%.2f")
            
            dpg.add_spacer(height=20)
            dpg.add_button(label="➕ Add Strategy Configuration to Batch", width=-1, height=35, callback=add_sim_to_batch_callback)
            dpg.add_spacer(height=10)
            dpg.add_button(label="⚡ EXECUTE ENTIRE RUN BATCH PIPELINE", width=-1, height=45, callback=run_batch_pipeline_callback)

        # --- RIGHT PANEL: Shared Instance Context Pools & Batch Sim Queue ---
        with dpg.child_window(width=-1, height=670, label="Infrastructure Resource Registers"):
            
            dpg.add_text("2. Shared System Infrastructure Resource Objects (Drag items from here to the left)", color=[100, 255, 100])
            dpg.add_spacer(height=5)
            
            with dpg.group(horizontal=True):
                # Accounts Node Column View List
                with dpg.child_window(width=250, height=180, label="Account Hub"):
                    dpg.add_text("Available Accounts Pool", color=[200, 200, 200])
                    dpg.add_separator()
                    for acct in global_accounts:
                        lbl = dpg.add_selectable(label=f"💳 {acct}")
                        with dpg.drag_payload(parent=lbl, payload_type="ACCOUNT_TYPE_PAYLOAD", drag_data=acct):
                            dpg.add_text(f"Moving Account: {acct}")
                            
                # Brokers Node Column View List
                with dpg.child_window(width=250, height=180, label="Broker Hub"):
                    dpg.add_text("Available Brokers Pool", color=[200, 200, 200])
                    dpg.add_separator()
                    for broker in global_brokers:
                        lbl = dpg.add_selectable(label=f"💼 {broker}")
                        with dpg.drag_payload(parent=lbl, payload_type="BROKER_TYPE_PAYLOAD", drag_data=broker):
                            dpg.add_text(f"Moving Broker: {broker}")

                # Feeds Node Column View List
                with dpg.child_window(width=250, height=180, label="Feeds Hub"):
                    dpg.add_text("Available Data Tapes", color=[200, 200, 200])
                    dpg.add_separator()
                    for feed in global_feeds:
                        lbl = dpg.add_selectable(label=f"📈 {feed}")
                        with dpg.drag_payload(parent=lbl, payload_type="FEED_TYPE_PAYLOAD", drag_data=feed):
                            dpg.add_text(f"Moving Feed Link: {feed}")

            dpg.add_spacer(height=20)
            dpg.add_text("3. Target Multi-Simulation Processing Pipeline Batch Queue", color=[100, 200, 255])
            dpg.add_separator()
            
            with dpg.table(header_row=True, tag="batch_queue_table", borders_innerH=True, borders_outerH=True, borders_innerV=True, borders_outerV=True, resizable=True):
                dpg.add_table_column(label="Simulation Instance ID")
                dpg.add_table_column(label="Strategy Rule Class")
                dpg.add_table_column(label="Mapped Account Target")
                dpg.add_table_column(label="Mapped Broker Gateway")
                dpg.add_table_column(label="Subscribed Feeding Data Streams")
                
                for sim in active_batch_simulations:
                    with dpg.table_row():
                        dpg.add_text(sim["sim_id"])
                        dpg.add_text(sim["strat"])
                        dpg.add_text(sim["acct"])
                        dpg.add_text(sim["broker"])
                        dpg.add_text(", ".join(sim["feeds"]))


# =========================================================================
# 📊 VIEW 2: MULTI-SIM BATCH ANALYTICS RESULTS OVERVIEW PANEL
# =========================================================================
with dpg.window(label="Simulation Analytics", tag="results_window", no_move=True, no_resize=True, no_title_bar=True, show=False):
    with dpg.group(horizontal=True):
        dpg.add_button(label="⬅ Back to Workbench", callback=show_home_screen)
        dpg.add_text("BATCH SEQUENCE ANALYSIS MONITORING PANEL", color=[100, 200, 255])
        
    dpg.add_separator()
    
    with dpg.group(horizontal=True):
        with dpg.child_window(width=280, height=630, label="Runs Selector Index Panel"):
            dpg.add_text("Completed Run Targets", color=[255, 200, 100])
            dpg.add_separator()
            #select from current list
            dpg.add_selectable(label="📊 Sim_Donchian_AAPL (Active)", default_value=True)
            dpg.add_selectable(label="📊 Sim_SMACross_MSFT")
            
        with dpg.child_window(width=-1, height=630, label="Dynamic Metrics Plots Workspace View Container"):
            with dpg.group(horizontal=True):
                with dpg.child_window(width=220, height=70):
                    dpg.add_text("CAGR", color=[200, 200, 200])
                    dpg.add_text("5.23 %", tag="ui_metric_cagr", color=[100, 255, 100])
                with dpg.child_window(width=220, height=70):
                    dpg.add_text("Sharpe Ratio", color=[200, 200, 200])
                    dpg.add_text("1.14", tag="ui_metric_sharpe", color=[100, 255, 100])
                with dpg.child_window(width=220, height=70):
                    dpg.add_text("Max Drawdown", color=[200, 200, 200])
                    dpg.add_text("-12.42 %", tag="ui_metric_dd", color=[255, 100, 100])
                    
            dpg.add_spacer(height=10)
            
            with dpg.group(horizontal=True):
                with dpg.child_window(width=600, height=500):
                    dpg.add_text("Account Equity Tracking & Value Curves Canvas Stack Area", color=[255, 200, 100])
                    with dpg.child_window(height=220, label="Equity Plot Line Box"):
                        dpg.add_text("[Equity Time-Series Interactive Line Graph Plot Canvas Node Area]")
                    dpg.add_spacer(height=5)
                    with dpg.child_window(height=220, label="Drawdown Plot Line Box"):
                        dpg.add_text("[Drawdown Trailing Deviation Shaded Area Graph Plot Canvas Node Area]")
                        
                with dpg.child_window(width=-1, height=500, label="Ledger Ledger"):
                    dpg.add_text("Run Specific Executed Trades Ledger Array Log", color=[100, 255, 255])
                    dpg.add_separator()
                    with dpg.table(header_row=True, borders_innerH=True, borders_outerH=True, borders_innerV=True, borders_outerV=True, resizable=True):
                        dpg.add_table_column(label="Index")
                        dpg.add_table_column(label="Action")
                        dpg.add_table_column(label="Execution Price")


def resize_windows_handler():
    width = dpg.get_viewport_width()
    height = dpg.get_viewport_height()
    dpg.configure_item("home_screen_window", width=width-16, height=height-38)
    dpg.configure_item("results_window", width=width-16, height=height-38)

dpg.create_viewport(title='Distributed Backtesting Engine Advanced Queue Management Dashboard', width=1300, height=820)
dpg.setup_dearpygui()
dpg.show_viewport()

dpg.set_viewport_resize_callback(resize_windows_handler)
resize_windows_handler()

dpg.start_dearpygui()
dpg.destroy_context()