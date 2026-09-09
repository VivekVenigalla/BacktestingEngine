#this is the GUI's entry point - running "python3 GUI.py" builds every
#window/widget defined below(most of it at import time, not inside a
#function - dearpygui builds its whole ui tree as a side effect of the code
#in this file simply running top to bottom) and then hands control over to
#dearpygui's own event loop at the very bottom(dpg.start_dearpygui()),
#which is what actually keeps the window open and responds to clicks
#
#dearpygui itself: it's an "immediate-ish" ui library where you build a
#tree of items(windows, buttons, text...) using "with" blocks that nest the
#way the widgets visually nest, and every item can be given a unique "tag"
#(a string or int id) so other code can find/update/delete it later by
#name - that tag-based lookup(dpg.does_item_exist, dpg.get_value,
#dpg.set_value, dpg.configure_item, dpg.delete_item...) is the core pattern
#used throughout this whole file
import dearpygui.dearpygui as dpg
import core  #import core functions
from csv_download import downloadData
from datetime import datetime
import math
import yfinance as yf
import os
import json
import subprocess
import threading
import sys

#every dearpygui program needs a context created before any ui items can be
#built - this has to run once, near the top, before anything else below
dpg.create_context()

#active batch with a deafult id
active_batch = {
    "simulation_metadata": {"batch_id": "batch_123", "notes": ""},
    "account": [],
    "broker": [],
    "data_feeds": [],
    "simulations": []
}

#NOTE in dearpygui, windows are the main canvas, while viewports are the individual sections you see
#this project shows one of these three "windows"(really, full-screen pages)
#at a time and hides the other two - see route_to_view() right below,
#which is just a loop that shows the one matching target_window_tag and
#hides everything else in this list
windows = ["landing_hub_window", "workbench_window", "config_manager_window"]

# =============================================================
# HELPER FUNCTIONS
# =============================================================

#functions for going between windows and also resizing
def route_to_view(target_window_tag):
    for view in windows:
        if dpg.does_item_exist(view):
            #"show=(view == target_window_tag)" evaluates the comparison
            #first(True/False) and passes that straight in as the show
            #argument - a compact way of writing "show it if this is the
            #target, hide it otherwise" without an if/else
            dpg.configure_item(view, show=(view == target_window_tag))
def resize_windows_handler():
    #keeps whichever "page" is currently visible sized to fill the actual
    #os window - this runs once at startup and again every time the user
    #resizes the app window(see dpg.set_viewport_resize_callback near the
    #bottom of this file)
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
#this one small function is reused everywhere in this file for every popup
#- plain info messages, error messages, and yes/no confirmation dialogs are
#all really just this same modal window with different buttons wired up,
#instead of separate, hand-built windows for each case
def spawn_message_modal(title, message, is_error=False, confirm_callback=None, callback_data=None, need_ok = False):
    #generate a unique integer ID
    #dpg.generate_uuid() hands back a fresh, guaranteed-unique id - used as
    #this modal's tag so multiple modals can exist without their tags
    #colliding(you can't have two dearpygui items sharing one tag)
    modal_tag = dpg.generate_uuid()

    #use a conditional statement to change the color if there is a error
    text_color = [255, 100, 100] if is_error else [255, 255, 255]

    #"with dpg.window(...):" opens a new window and makes everything
    #indented underneath it a CHILD of that window - this nesting-by-
    #indentation is how dearpygui builds up its whole visual tree
    #throughout this file
    with dpg.window(label=title, tag=modal_tag, modal=True, show=True, no_collapse=True, no_resize=True, width=350):
        dpg.add_text(message, color=text_color, wrap=330)
        dpg.add_spacer(height=10)
        dpg.add_separator()
        dpg.add_spacer(height=5)
        
        with dpg.group(horizontal=True):
            #if we need a confirmation create confirmation buttons and cancel
            if confirm_callback:
                #"def wrapper_yes():" defines a small function nested
                #INSIDE another function - a closure. it can see and use
                #confirm_callback/callback_data/modal_tag from the outer
                #spawn_message_modal call even after that outer call has
                #already returned, because python keeps those variables
                #alive as long as something(the button's callback here)
                #still references them
                def wrapper_yes():
                    confirm_callback(callback_data)
                    dpg.delete_item(modal_tag)

                dpg.add_button(label="Yes, Proceed", callback=wrapper_yes, width=160)
                #"callback=lambda: dpg.delete_item(modal_tag)" is a lambda
                #used as a one-line inline callback - same idea as
                #wrapper_yes above, just short enough to skip giving it its
                #own def/name. dearpygui calls whatever function you hand
                #it to "callback" the moment that button is clicked
                dpg.add_button(label="Cancel", callback=lambda: dpg.delete_item(modal_tag), width=160)
            elif is_error or need_ok:
                #error notice message
                dpg.add_spacer(width=135)
                dpg.add_button(label="OK", callback=lambda: dpg.delete_item(modal_tag), width=60)


    vp_width = dpg.get_viewport_client_width()
    vp_height = dpg.get_viewport_client_height()
    #"//" is integer(floor) division - vp_width // 2 always gives a whole
    #number, since dpg.set_item_pos expects whole pixel coordinates rather
    #than a fractional one like plain "/" division could produce
    dpg.set_item_pos(modal_tag, [vp_width // 2 - 175, vp_height // 2 - 75])
    return modal_tag

# =============================================================
# CONFIG MODALS
# =============================================================
#the next three modals(account/broker/feed) all follow the same "create OR
#edit" pattern: user_data is None when creating something brand new, or the
#existing object's dict when editing one(see the Edit buttons in
#refresh_config_manager_tables further down, which pass the row's data in
#as user_data) - is_edit just checks which situation this call is
def spawn_create_account_modal(sender=None, app_data=None, user_data=None):
    #every dearpygui callback function is expected to accept(sender,
    #app_data, user_data) - sender is the tag of whatever item triggered
    #the callback, app_data is event-specific data dearpygui passes
    #automatically, and user_data is whatever you attached yourself when
    #registering the callback(e.g "callback=spawn_create_account_modal,
    #user_data=acct" further down in this file). the "=None" defaults let
    #you also call this function directly with no arguments at all(e.g a
    #plain button click with no user_data attached)
    close_modal("modal_create_account")
    is_edit = user_data is not None
    data = user_data
    init_id = data["id"] if is_edit else "newAccount"
    init_bal = data["initial_balance"] if is_edit else 10000.0
    init_reset = data["reset"] if is_edit else True

    with dpg.window(label="Create New Global Account", tag="modal_create_account", modal=True, width=350, height=200):
        #every input widget below is given a fixed tag(e.g "m_acct_id") so
        #the save() closure right after can read back whatever the user
        #typed via dpg.get_value(tag) - dearpygui doesn't hand you the
        #current value automatically, you always fetch it by tag when you
        #need it
        dpg.add_input_text(label="Account ID", tag="m_acct_id", default_value=init_id)
        dpg.add_input_float(label="Initial Balance", tag="m_acct_bal", default_value=init_bal)
        dpg.add_checkbox(label="Reset", tag="m_acct_reset", default_value=init_reset)

        #save() is another closure(see wrapper_yes above for what that
        #means) - it's defined here so it can reach is_edit/data from the
        #enclosing spawn_create_account_modal call, and gets wired up as
        #the Save button's callback a few lines down
        def save():
            new_acc = {
                "id": dpg.get_value("m_acct_id"),
                "initial_balance": dpg.get_value("m_acct_bal"),
                "reset": dpg.get_value("m_acct_reset")
            }
            close_modal("modal_create_account")
            #split frame waits for the close call to finish
            #dpg.split_frame() tells dearpygui "finish rendering the
            #current frame before running anything else" - without it, a
            #delete_item followed immediately by another window being
            #shown can race against dearpygui's own rendering and glitch
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
                #enumerate(list) walks a list handing back both the index
                #AND the item together(idx, a) - needed here specifically
                #because a plain "for a in ...: a = new_acc" would only ever
                #reassign the local loop variable, not the actual list
                #entry; you need the index to overwrite the real slot in
                #the list below
                for idx, a in enumerate(core.state["registered_accounts"]["account"]):
                    if a["id"] == new_acc["id"]:
                        core.state["registered_accounts"]["account"][idx] = new_acc
                        core.update_account_config()
            else:
                core.save_account_config(new_acc)
            refresh_all_ui()
            close_modal(logger_id)
            dpg.split_frame()
            #f-strings can nest a plain double-quoted string INSIDE another
            #double-quoted f-string like this(a python 3.12+ feature) -
            #older python would need the inner quotes to be single quotes
            #instead(f'Account {new_acc["id"]} saved') to avoid ending the
            #string early
            logger_id = spawn_message_modal("Logger", f"Account {new_acc["id"]} saved", need_ok = True)

        with dpg.group(horizontal=True):
            dpg.add_button(label="Save Object", callback=save)
            dpg.add_button(label="Cancel", callback=lambda: close_modal("modal_create_account"))

#same create/edit pattern as spawn_create_account_modal above - see that
#one for what is_edit/get_value/split_frame/enumerate are doing here too
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
    #heads up: this whole broker->account link ui was disabled(the combo
    #box below and reading its value back into new_brk are both commented
    #out) - a broker's account_link only actually gets attached later,
    #implicitly, when the workbench canvas is compiled into a batch json
    #(see parse_workbench_canvas's "brk_obj["account_link"] = acct_obj["id"]"
    #line further down). that means editing a broker through THIS modal
    #can silently drop an account_link a batch export had already set
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

#same create/edit pattern again, plus a date picker this time - dearpygui's
#date picker widget hands back a dict{'month_day', 'month', 'year'} rather
#than a python datetime, and quirkily counts years as an offset from
#1900 and months starting at 0(january), so there's some format-juggling
#below to convert to/from real "YYYY-MM-DD" strings the rest of the
#project(and datetime.strptime, which parses a string into a real datetime
#object using a format pattern) actually expects
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
            #the ":04d"/":02d" inside these f-string {} braces are format
            #specs - "04d" pads a whole number out to 4 digits with leading
            #zeros(so year 24 becomes "0024", not what we want here since
            #+1900 already gives a real 4-digit year, but month/day both
            #use "02d" to get "01" instead of a bare "1")
            start_str = f"{raw_start['year'] + 1900:04d}-{raw_start['month'] + 1:02d}-{raw_start['month_day']:02d}"
            end_str = f"{raw_end['year'] + 1900:04d}-{raw_end['month'] + 1:02d}-{raw_end['month_day']:02d}"

            #to get cagr length we need to parse the string as a datetime object and then use that to find the percentage of a year
            d1 = datetime.strptime(start_str, "%Y-%m-%d")
            d2 = datetime.strptime(end_str, "%Y-%m-%d")
            #subtracting two datetimes gives a timedelta object; .days pulls
            #the whole-number day count back out of it. abs() guards
            #against a negative count if the dates were ever picked in the
            #wrong order(start after end)
            days_between = abs((d2 - d1).days)


            #use math.ceil to use only integers for the length of years elapsed
            #math.ceil always rounds UP to the next whole number(2.1
            #becomes 3), unlike normal rounding - so even a date range one
            #day over a full year counts as a full extra year for cagr
            #purposes
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
            #this makes an actual live network call to yahoo finance just
            #to validate the ticker string is real BEFORE letting the user
            #save the feed - a fake ticker like "ZZZZZ" won't error out
            #immediately, it just comes back with an EMPTY dataframe(hist),
            #which is why the emptiness check below is what catches it,
            #not the try/except alone
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
                            #heads up, likely bug: user_data["csv_filepath"]
                            #is already stored as a path like
                            #"../data/MSFT_1D_....csv"(see how new_fd builds
                            #it above), but data_dir here is ALSO "../data" -
                            #joining them produces a doubled-up path like
                            #"../data/../data/MSFT_....csv". the
                            #os.path.exists check just below this likely
                            #fails silently on that malformed path, meaning
                            #the OLD csv file for an edited feed never
                            #actually gets deleted
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
# NODE EDITING
# =============================================================

#nodes work with nodes being the different sections and the attributes being the connections betwee nodes
#dearpygui's node_editor is a whole separate widget system on top of the
#regular ui: a "node"(dpg.node) is one draggable box on the canvas, and
#each node contains one or more "node_attribute"s, which are the actual pin
#connection points on its edges. an attribute_type of Input/Output decides
#which side of the box the pin renders on and which direction a wire can
#run(Static is used for attributes that are just informational text, with
#no pin at all - see the balance/reset display below). every node/
#attribute here also gets a "user_data" dict attached to it - that's not a
#dearpygui built-in concept, it's this project's own convention for
#stashing "what kind of thing is this, and what's its underlying account/
#broker/feed/strategy data" directly on the visual widget, so later code
#(on_node_link_created, parse_workbench_canvas...) can read it back off
#whatever node/pin the user clicked or connected

def spawn_account_node(account_data, pos = [50,50]):
    #generate a unique id for both the tag and the attribute
    node_tag = dpg.generate_uuid()
    out_attr_tag = dpg.generate_uuid()

    #parent="node_editor_canvas" is what places this new node onto the
    #actual node-editor widget(defined once, further down, in VIEWPORT 3) -
    #dearpygui lets you attach a new item to an existing parent by tag even
    #from a totally different function, not just via nesting "with" blocks
    with dpg.node(parent="node_editor_canvas", label=f"Account: {account_data['id']}", tag=node_tag, pos=pos):
        # Store metadata inside user_data
        dpg.set_item_user_data(node_tag, {"type": "ACCOUNT", "data": account_data})

        #a Static attribute has no connectable pin - it's just used here to
        #show a couple of read-only text lines inside the node's body
        with dpg.node_attribute(attribute_type=dpg.mvNode_Attr_Static):
            dpg.add_text(f"Initial Balance: ${account_data['initial_balance']}")
            dpg.add_text(f"Reset on Run: {account_data['reset']}")

        #output pin for strategy and broker
        #this pin's own user_data({"pin_type": "ACCOUNT"}) is separate from
        #the node's user_data set above - it's what on_node_link_created
        #checks to enforce "an ACCOUNT output can only connect to an
        #ACCOUNT-shaped input", further down this file
        with dpg.node_attribute(tag=out_attr_tag, attribute_type=dpg.mvNode_Attr_Output, user_data={"pin_type": "ACCOUNT"}):
            dpg.add_text("Account Link ->", color=[100, 200, 255])
    return node_tag


#same node/attribute/user_data pattern as spawn_account_node above, just
#with one INPUT pin(account) as well as one output pin this time
def spawn_broker_node(broker_data, pos = [300,50]):
    node_tag = dpg.generate_uuid()
    #broker requires one in attrribute 
    in_acct_tag = dpg.generate_uuid()
    out_broker_tag = dpg.generate_uuid()
    
    with dpg.node(parent="node_editor_canvas", label=f"Broker: {broker_data['id']}", tag=node_tag, pos=pos):
        dpg.set_item_user_data(node_tag, {"type": "BROKER", "data": broker_data})
        
        #input pin for account link
        with dpg.node_attribute(tag=in_acct_tag, attribute_type=dpg.mvNode_Attr_Input,user_data={"pin_type": "ACCOUNT"}):
            dpg.add_text("<- Account Link", color=[100, 200, 255])
            
        with dpg.node_attribute(attribute_type=dpg.mvNode_Attr_Static):
            dpg.add_text(f"Commission: {broker_data['commission_rate']}")
            dpg.add_text(f"Slippage: {broker_data['slippage_rate']}")
            
        #ouptut pin for strategy
        with dpg.node_attribute(tag=out_broker_tag, attribute_type=dpg.mvNode_Attr_Output,user_data={"pin_type": "BROKER"}):
            dpg.add_text("Broker Link ->", color=[255, 200, 100])
    return node_tag


#same pattern once more, output-only(a feed only ever feeds INTO a strategy)
def spawn_feed_node(feed_data, pos = [50,300]):
    node_tag = dpg.generate_uuid()
    out_feed_tag = dpg.generate_uuid()
    
    with dpg.node(parent="node_editor_canvas", label=f"Feed: {feed_data['ticker']}", tag=node_tag, pos=pos):
        dpg.set_item_user_data(node_tag, {"type": "FEED", "data": feed_data})
        
        with dpg.node_attribute(attribute_type=dpg.mvNode_Attr_Static):
            dpg.add_text(f"Timeframe: {feed_data['timeframe']}")
            dpg.add_text(f"Range: {feed_data.get('start_date', '')} to {feed_data.get('end_date', '')}")
            
        #output pin for strategy
        with dpg.node_attribute(tag=out_feed_tag, attribute_type=dpg.mvNode_Attr_Output,user_data={"pin_type": "FEED"}):
            dpg.add_text("Feed Link ->", color=[100, 255, 100])
    return node_tag

#the strategy node is the busiest one - three input pins(account, broker,
#feed) PLUS a block of parameter widgets that are built dynamically based
#on whatever "parameters" list came from strategyBlueprints.json for this
#strategy(see the createStrat.cpp/strategyBlueprints.json side of this same
#contract - {"name","type","default"} objects), rather than being hardcoded
#per strategy type here
def spawn_strategy_node(strategy_data, pos = [600,150]):
    node_tag = dpg.generate_uuid()
    in_acct_tag = dpg.generate_uuid()
    in_broker_tag = dpg.generate_uuid()
    in_feed_tag = dpg.generate_uuid()

    with dpg.node(parent="node_editor_canvas", label=f"Strategy: {strategy_data['display_name']}", tag=node_tag, pos=pos):
        dpg.set_item_user_data(node_tag, {"type": "STRATEGY", "data": strategy_data})

        #input pins for account, broker, and feed
        with dpg.node_attribute(tag=in_acct_tag, attribute_type=dpg.mvNode_Attr_Input,user_data={"pin_type": "ACCOUNT"}):
            dpg.add_text("<- Account Link", color=[100, 200, 255])

        with dpg.node_attribute(tag=in_broker_tag, attribute_type=dpg.mvNode_Attr_Input,user_data={"pin_type": "BROKER"}):
            dpg.add_text("<- Broker Link", color=[255, 200, 100])

        with dpg.node_attribute(tag=in_feed_tag, attribute_type=dpg.mvNode_Attr_Input,user_data={"pin_type": "FEED"}):
            dpg.add_text("<- Feed Link(s)", color=[100, 255, 100])

        #inputs for parameters
        with dpg.node_attribute(attribute_type=dpg.mvNode_Attr_Static):
            #node_tag is a plain integer(from generate_uuid), so "% 1000"
            #(modulo - the remainder after dividing by 1000) just chops it
            #down to a short 3-digit-ish number for a friendlier-looking
            #default sim id, rather than the full, much longer raw uuid
            default_sim_id = f"{strategy_data['id']}_instance_{node_tag % 1000}"
            dpg.add_input_text(
                label="Sim ID",
                default_value=default_sim_id,
                width=130,
                tag=f"sim_id_{node_tag}"
            )

            # 2. Run by Default Checkbox
            dpg.add_checkbox(
                label="Run Default",
                default_value=True,
                tag=f"run_default_{node_tag}"
            )



            #dpg.add_separator()
            dpg.add_text("Parameters(Overridable):", color=[200, 200, 200])

            # Dynamically draw parameter controls based on type
            #this loop is what makes the node's parameter section adapt to
            #whatever strategy it represents - each parameter's json
            #"type" field picks which kind of input widget to draw
            #(int/float/bool/anything else falls back to plain text), and
            #each widget's tag encodes both this node's id and the
            #parameter's name(f"param_{node_tag}_{p_name}") so
            #parse_workbench_canvas can later find and read back every
            #parameter for every strategy node on the canvas, just by
            #reconstructing that same tag pattern
            for param in strategy_data.get("parameters", []):
                p_name = param["name"]
                p_type = param.get("type", "float")
                p_default = param.get("default", 0)

                param_tag = f"param_{node_tag}_{p_name}"

                if p_type == "int":
                    dpg.add_input_int(label=p_name, default_value=int(p_default), width=120, tag=param_tag)
                elif p_type == "float":
                    dpg.add_input_float(label=p_name, default_value=float(p_default), width=120, tag=param_tag)
                elif p_type == "bool":
                    dpg.add_checkbox(label=p_name, default_value=bool(p_default), tag=param_tag)
                else:
                    dpg.add_input_text(label=p_name, default_value=str(p_default), width=120, tag=param_tag)
    return node_tag

def clear_workbench_canvas():
    #removes all objects on the canvas
    if dpg.does_item_exist("node_editor_canvas"):
        #children only deletes the nodes and links and not the canvas itself
        dpg.delete_item("node_editor_canvas", children_only=True)


#this is the reverse of parse_workbench_canvas further down(which turns
#nodes+links INTO a batch json) - this function takes an existing batch
#json and rebuilds the equivalent nodes+links back onto the canvas, so a
#previously-saved batch can be reopened and edited visually
def load_batch_file_to_workbench(file_path):
    #parse an existing batch and switch to workbench view, while also creating the nodes and connections
    if not os.path.exists(file_path):
        spawn_message_modal("Load Error", f"Batch file not found:\n{file_path}", is_error=True)
        return

    try:
        with open(file_path, "r") as f:
            #load data
            batch_data = json.load(f)
    except Exception as e:
        spawn_message_modal("Load Error", f"Failed to parse batch JSON:\n{repr(e)}", is_error=True)
        return

    #change view to workbench
    route_to_view("workbench_window")
    clear_workbench_canvas()

    #get the metadata fields and set the respective fields to them
    meta = batch_data.get("simulation_metadata", {})
    if dpg.does_item_exist("ui_batch_id"):
        dpg.set_value("ui_batch_id", meta.get("batch_id", "loaded_batch"))
    if dpg.does_item_exist("ui_batch_notes"):
        dpg.set_value("ui_batch_notes", meta.get("notes", ""))

    #track nodes to establish links
    #these are maps for all of the of nodes and their temp node id: entity_id -> node_tag
    #not to be confused with config id, node id are the id used to find the visual node
    spawned_accounts = {}
    spawned_brokers = {}
    spawned_feeds = {}

    x_acct, x_brk, x_feed, x_strat = 50, 300, 50, 600
    y_acct, y_brk, y_feed, y_strat = 50, 50, 300, 150

    #spawn accounts
    for acct_data in batch_data.get("account", []):
        node_tag = spawn_account_node(acct_data, pos=[x_acct, y_acct])
        spawned_accounts[acct_data["id"]] = node_tag
        y_acct += 180

    #spawn brokers
    for brk_data in batch_data.get("broker", []):
        node_tag = spawn_broker_node(brk_data, pos=[x_brk, y_brk])
        spawned_brokers[brk_data["id"]] = node_tag
        y_brk += 200

        acct_id = brk_data.get("account_link")
        if acct_id in spawned_accounts:
            link_nodes_by_pin_type(spawned_accounts[acct_id], node_tag, "ACCOUNT")

    #spawn feeds
    for feed_data in batch_data.get("data_feeds", []):
        node_tag = spawn_feed_node(feed_data, pos=[x_feed, y_feed])
        spawned_feeds[feed_data["id"]] = node_tag
        y_feed += 180

    #"{st["id"]: st for st in ...}" is a dict comprehension - the dict
    #version of the list comprehensions used in core.py, building a
    #{strategy_id: full_strategy_dict} lookup table in one line instead of
    #a manual for-loop with dict[key] = value assignments
    registered_strats = {
        st["id"]: st for st in core.state.get("registered_strategies", {}).get("strategies", [])
    }

    #spawn strategies
    for sim in batch_data.get("simulations", []):
        strat_key = sim.get("strategy")
        base_strat = registered_strats.get(strat_key, {"id": strat_key, "display_name": strat_key, "parameters": []})

        strat_node_tag = spawn_strategy_node(base_strat, pos=[x_strat, y_strat])
        y_strat += 280

        #override parameters
        if dpg.does_item_exist(f"sim_id_{strat_node_tag}"):
            dpg.set_value(f"sim_id_{strat_node_tag}", sim.get("id", f"{strat_key}_sim"))
        if dpg.does_item_exist(f"run_default_{strat_node_tag}"):
            dpg.set_value(f"run_default_{strat_node_tag}", sim.get("run_all_by_default", True))

        for p_name, p_val in sim.get("parameters", {}).items():
            param_tag = f"param_{strat_node_tag}_{p_name}"
            if dpg.does_item_exist(param_tag):
                dpg.set_value(param_tag, p_val)

        #link account to strategy
        acct_id = sim.get("account_link")
        if acct_id in spawned_accounts:
            link_nodes_by_pin_type(spawned_accounts[acct_id], strat_node_tag, "ACCOUNT")

        #link broker to strategy
        brk_id = sim.get("broker_link")
        if brk_id in spawned_brokers:
            link_nodes_by_pin_type(spawned_brokers[brk_id], strat_node_tag, "BROKER")

        #link feed to strategy
        for feed_id in sim.get("feeds", []):
            if feed_id in spawned_feeds:
                link_nodes_by_pin_type(spawned_feeds[feed_id], strat_node_tag, "FEED")

def find_node_attribute(node_tag, pin_type, is_output=False):

    #find the attribute id for the the matching pin type an direction
    if not dpg.does_item_exist(node_tag):
        return None

    #get one slot children
    #dpg.get_item_children(node_tag, 1) returns just the items in "slot 1"
    #of this node - dearpygui items can have several separate child slots
    #for different purposes, and slot 1 is specifically where a node's own
    #node_attribute children live. "or []" falls back to an empty list if
    #this call ever returns None, so the for loop below never crashes on a
    #node with no children at all
    children = dpg.get_item_children(node_tag, 1) or []
    target_attr_type = dpg.mvNode_Attr_Output if is_output else dpg.mvNode_Attr_Input

    for child_id in children:
        #dpg.get_item_type gives back a string identifying what KIND of
        #widget an item is - this filters down to just the node_attribute
        #children(skipping over the dpg.add_text items etc that live
        #inside them)
        if dpg.get_item_type(child_id) == "mvAppItemType::mvNodeAttribute":
            conf = dpg.get_item_configuration(child_id)
            attr_direction = conf.get("attribute_type")

            u_data = dpg.get_item_user_data(child_id) or {}
            attr_pin_type = u_data.get("pin_type")

            if attr_direction == target_attr_type and attr_pin_type == pin_type:
                return child_id

    return None


def link_nodes_by_pin_type(src_node, dst_node, pin_type):
    #a triple-quoted string placed as the very first thing inside a
    #function is a real docstring(not just a comment) - tools/editors can
    #show it as this function's official documentation, unlike a regular
    #comment which they generally can't
    """
    Finds the output pin on src_node and input pin on dst_node
    for the specified pin_type and creates a visual link wire.
    """
    src_attr = find_node_attribute(src_node, pin_type, is_output=True)
    dst_attr = find_node_attribute(dst_node, pin_type, is_output=False)

    if src_attr and dst_attr:
        dpg.add_node_link(src_attr, dst_attr, parent="node_editor_canvas")
        print(f"[UI Rebuild Link Success] {pin_type}: Node {src_node} -> Node {dst_node}")
        return True
    else:
        print(f"[UI Rebuild Link Failed] Missing Pin for {pin_type}: src_attr={src_attr}, dst_attr={dst_attr}")
        return False

def get_pin_info(attr_tag):
    #retrieve the entity data and parent node
    if not dpg.does_item_exist(attr_tag):
        return None, None
    node_tag = dpg.get_item_parent(attr_tag)
    if not dpg.does_item_exist(node_tag):
        return None, None
    node_data = dpg.get_item_user_data(node_tag)
    return node_tag, node_data


def get_all_active_links():
    #get a list of all node link items on the canvas
    all_items = dpg.get_all_items()
    links = []
    for item in all_items:
        #check if the item is a node link
        if dpg.get_item_type(item) == "mvAppItemType::mvNodeLink":
            links.append(item)
    return links


#this is the callback wired up to the node_editor's own "callback"(see
#VIEWPORT 3's dpg.node_editor(...) call) - dearpygui runs it automatically
#every time the user drags a wire between two pins, BEFORE the wire is
#actually drawn. all of the validation below decides whether
#dpg.add_node_link actually gets called at the very end(and therefore
#whether the wire appears at all) - if any check fails and returns early,
#the connection is silently rejected
def on_node_link_created(sender, app_data):

    #when two nodes are connected, this is executed
    #app data is the start and end node
    #app_data arrives as a 2-item tuple(the two pins involved); "attr_start,
    #attr_end = app_data" is tuple unpacking - assigning each element of a
    #tuple/list to its own named variable in one line
    attr_start, attr_end = app_data

    node_start, data_start = get_pin_info(attr_start)
    node_end, data_end = get_pin_info(attr_end)
    
    #if there is any missing data send a error
    if not data_start or not data_end:
        print("[UI Link Error] Missing user data on connected nodes.")
        dpg.split_frame()
        spawn_message_modal("Node Error", f"Missing data on connected nodes", is_error=True)
        return

    #get the type of each nodes
    type_start = data_start.get("type")
    type_end = data_end.get("type")
    
    #valid connection rules (first -> second)
    #a tuple like ("ACCOUNT", "BROKER") can be compared for equality and
    #used with "in"/"not in" just like any other value - bundling
    #type_start/type_end into a tuple() below and checking it against this
    #whole list of valid (from, to) pairs in one line is shorter than
    #writing out every combination as a separate if/or condition
    valid_connections = [
        ("ACCOUNT", "BROKER"),
        ("ACCOUNT", "STRATEGY"),
        ("BROKER", "STRATEGY"),
        ("FEED", "STRATEGY")
    ]

    #check link is in the valid list
    if (type_start, type_end) not in valid_connections:
        print(f"[UI Link Blocked] Invalid Connection: {type_start} -> {type_end}")
        dpg.split_frame()
        spawn_message_modal("Link Error", f"Invalid Connection: {type_start} -> {type_end}", is_error=True)
        return

    #check if the connection is made at the right spots
    #get user data of the pin
    dst_attr_data = dpg.get_item_user_data(attr_end) or {}
    required_pin_type = dst_attr_data.get("pin_type")

    #check if the incoming pin type matches the input pin
    if type_start != required_pin_type:
        print(f"[UI Link Blocked] Cannot connect {type_start} output to {required_pin_type} input pin.")
        dpg.split_frame()
        spawn_message_modal("Link Error", f"Cannot connect {type_start} output to {required_pin_type} input pin", is_error=True)
        return
    #get all active links
    active_links = get_all_active_links()

    #if it is not a feed -> strategy, there is a limit of one link
    if (type_start, type_end) in [("ACCOUNT", "BROKER"), ("ACCOUNT", "STRATEGY"), ("BROKER", "STRATEGY")]:
        for link_id in active_links:
            #this returns the values in the set_node_data
            conf = dpg.get_item_configuration(link_id)
            exist_start_attr = conf.get("attr_1")
            exist_end_attr = conf.get("attr_2")
            
            exist_src_node, exist_src_data = get_pin_info(exist_start_attr)
            exist_dst_node, exist_dst_data = get_pin_info(exist_end_attr)
            
            #check if target node already has a link active to it
            if exist_dst_node == node_end and exist_src_data and exist_src_data.get("type") == type_start:
                print(f"[UI Link Blocked] Node already has a connected {type_start}.")
                dpg.split_frame()
                spawn_message_modal("Link Error", f"Node already has a connected {type_start}", is_error=True)
                return

    #if it is a feed to strategy, use feedNum
    elif type_start == "FEED" and type_end == "STRATEGY":
        strat_info = data_end.get("data", {})
        feed_num = strat_info.get("feedNum", 1)
        
        #count incoming feed links to strategy node
        feed_link_count = 0
        for link_id in active_links:
            conf = dpg.get_item_configuration(link_id)
            exist_start_attr = conf.get("attr_1")
            exist_end_attr = conf.get("attr_2")
            
            exist_dst_node, _ = get_pin_info(exist_end_attr)
            if exist_dst_node == node_end:
                #NOTE: _ is a placeholder that is not used
                _ , src_data = get_pin_info(exist_start_attr)
                if src_data and src_data.get("type") == "FEED":
                    #prevent duplicate link to the same strategy
                    exist_src_node, _ = get_pin_info(exist_start_attr)
                    if exist_src_node == node_start:
                        print(f"[UI Link Blocked] This Feed is already connected to this Strategy.")
                        dpg.split_frame()
                        spawn_message_modal("Link Error", f"This Feed is already connected to this Strategy", is_error=True)
                        return
                    feed_link_count += 1

        #positive feedNum, struct requirement
        if feed_num > 0 and feed_link_count >= feed_num:
            print(f"[UI Link Blocked] Strategy '{strat_info.get('id')}' only allows maximum {feed_num} feed(s).")
            dpg.split_frame()
            spawn_message_modal("Link Error", f"Strategy '{strat_info.get('id')}' only allows maximum {feed_num} feed(s)", is_error=True)
            return
            
        #negative feedNum, At least abs(feedNum) feeds required
        elif feed_num < 0:
            print(f"[UI Link Info] Strategy allows variable feeds (min {abs(feed_num)} required). Current feeds: {feed_link_count + 1}")

    # If all validation passes, spawn the link wire
    dpg.add_node_link(attr_start, attr_end, parent=sender)
    print(f"[UI Link Created] {type_start} -> {type_end}")

def on_node_link_deleted(sender, app_data):
    #app_data is the link tag ID
    dpg.delete_item(app_data)
    print(f"[UI Link Deleted] Link ID: {app_data}")


#this is the single most important function in the whole gui - it's the
#reverse of load_batch_file_to_workbench: walks every node and wire
#currently on the canvas and compiles them into the exact batch json
#structure the c++ engine expects(matching the shape you'd see in
#config/batchConfig/*.json). it returns a python tuple(batch_config,
#error_message) - by convention here, exactly one of the two is ever
#non-None: a success hands back (dict, None), a validation failure hands
#back (None, "some message"), so callers always check "if error_msg:"
#first before trusting batch_config
def parse_workbench_canvas():
    #return type : batch_config and error message if any

    #inspect the active nodes and links
    #validate all connections are correct, and compile them into the batch .json file
    if not dpg.does_item_exist("node_editor_canvas"):
        return None, "Node canvas does not exist."

    #dpg.get_all_items() returns literally every widget in the whole
    #program, not just this canvas - these two list comprehensions filter
    #that flat list down to just the mvNode/mvNodeLink items(the actual
    #boxes and wires), same "get_item_type == ..." filtering idea used in
    #find_node_attribute above
    all_items = dpg.get_all_items()
    nodes = [item for item in all_items if dpg.get_item_type(item) == "mvAppItemType::mvNode"]
    links = [item for item in all_items if dpg.get_item_type(item) == "mvAppItemType::mvNodeLink"]

    #map attributes to nodes
    attr_to_node = {}
    for node_id in nodes:
        children_groups = dpg.get_item_children(node_id) or {}
        for group_idx in children_groups:
            for child_id in children_groups[group_idx]:
                if dpg.get_item_type(child_id) == "mvAppItemType::mvNodeAttribute":
                    attr_data = dpg.get_item_user_data(child_id) or {}
                    attr_to_node[child_id] = {
                        "node_id": node_id,
                        "pin_type": attr_data.get("pin_type")
                    }

    #build the connection map of all source(parent) nodes of a pin type of a particular destination(child) node
    #graph structure: node_connections[dst_node][pint_type] = list of all parent node ids ; dst_node is the destination node

    node_connections = {}
    for link_id in links:
        conf = dpg.get_item_configuration(link_id)
        src_attr = conf.get("attr_1")
        dst_attr = conf.get("attr_2")

        if src_attr in attr_to_node and dst_attr in attr_to_node:
            src_info = attr_to_node[src_attr]
            dst_info = attr_to_node[dst_attr]

            dst_node = dst_info["node_id"]
            pin_type = dst_info["pin_type"]

            if dst_node not in node_connections:
                node_connections[dst_node] = {}
            if pin_type not in node_connections[dst_node]:
                node_connections[dst_node][pin_type] = []

            node_connections[dst_node][pin_type].append(src_info["node_id"])

    #categorize the nodes
    account_nodes = {}
    broker_nodes = {}
    feed_nodes = {}
    strategy_nodes = {}

    for node_id in nodes:
        user_data = dpg.get_item_user_data(node_id) or {}
        node_type = user_data.get("type")
        entity_data = user_data.get("data", {})

        if node_type == "ACCOUNT":
            account_nodes[node_id] = entity_data
        elif node_type == "BROKER":
            broker_nodes[node_id] = entity_data
        elif node_type == "FEED":
            feed_nodes[node_id] = entity_data
        elif node_type == "STRATEGY":
            strategy_nodes[node_id] = {"data": entity_data, "node_id": node_id}

    #if there are no strategy nodes
    if not strategy_nodes:
        return None, "Cannot build batch: No strategy nodes placed on canvas."

    compiled_simulations = []
    #a set() automatically de-duplicates whatever you add to it - used here
    #(rather than a list) because the same account/broker/feed can be
    #reused by multiple strategy nodes on the canvas, and each one should
    #only end up once in the final exported batch json, not once per
    #strategy that references it
    compiled_accounts = set()
    compiled_brokers = set()
    compiled_feeds = set()
    used_broker_objs = {}

    #process and validate strategy nodes
    for node_id, strat_meta in strategy_nodes.items():
        strat_data = strat_meta["data"]
        strat_key = strat_data["id"]
        feed_num_req = strat_data.get("feedNum", 1)

        conns = node_connections.get(node_id, {})

        #validate account link
        connected_acct_nodes = conns.get("ACCOUNT", [])
        #if the dict is empty that means there is no connected account node
        if not connected_acct_nodes:
            return None, f"Strategy '{strat_data.get('display_name')}' is missing an Account connection."
        acct_obj = account_nodes[connected_acct_nodes[0]]

        #validate broker connection
        connected_brk_nodes = conns.get("BROKER", [])
        if not connected_brk_nodes:
            return None, f"Strategy '{strat_data.get('display_name')}' is missing a Broker connection."
        brk_obj = broker_nodes[connected_brk_nodes[0]]
        #ensure brokers have account link
        brk_node_id = connected_brk_nodes[0]
        brk_conns = node_connections.get(brk_node_id, {})
        brk_acct_nodes = brk_conns.get("ACCOUNT", [])
        if not brk_acct_nodes:
            return None, f"Broker '{brk_obj.get('id')}' connected to Strategy '{strat_data.get('display_name')}' is missing an Account connection."

        #ensure broker objects have the same link as the strategy
        brk_acct_obj = account_nodes[brk_acct_nodes[0]]
        if brk_acct_obj["id"] != acct_obj["id"]:
            return None, f"Mismatched Accounts! Strategy '{strat_data.get('display_name')}' is linked to Account '{acct_obj['id']}', but Broker '{brk_obj['id']}' is linked to Account '{brk_acct_obj['id']}'."

        #this is where a broker actually gets its account_link set - see
        #spawn_create_broker_modal's heads-up comment above: the modal's own
        #account-link ui is disabled, so THIS line, run at export time, is
        #the only place a broker's account_link ever gets attached. since
        #brk_obj is the exact same dict object stored in broker_nodes(python
        #dicts are mutable and passed by reference, not copied), mutating it
        #here also updates what's already sitting in used_broker_objs below
        brk_obj["account_link"] = acct_obj["id"]
        used_broker_objs[brk_obj["id"]] = brk_obj
        #validate feed links and feedNum
        connected_feed_nodes = conns.get("FEED", [])
        actual_feed_count = len(connected_feed_nodes)

        #if feedNum is greater than 0, has to be precise
        if feed_num_req > 0 and actual_feed_count != feed_num_req:
            return None, f"Strategy '{strat_data.get('display_name')}' requires {feed_num_req} feed(s), but has {actual_feed_count} connected."
        #otherwise check if is greater than feedNum or not
        elif feed_num_req < 0 and actual_feed_count < abs(feed_num_req):
            return None, f"Strategy '{strat_data.get('display_name')}' requires at least {abs(feed_num_req)} feed(s), but only has {actual_feed_count} connected."

        #get all the entity data
        feed_objs = [feed_nodes[f_node] for f_node in connected_feed_nodes]

        sim_instance_id = dpg.get_value(f"sim_id_{node_id}") or f"{strat_key}_sim"
        run_default = dpg.get_value(f"run_default_{node_id}")

        #get overriden parameter values of strategy
        extracted_params = {}
        for param in strat_data.get("parameters", []):
            p_name = param["name"]
            param_tag = f"param_{node_id}_{p_name}"
            if dpg.does_item_exist(param_tag):
                extracted_params[p_name] = dpg.get_value(param_tag)
            else:
                extracted_params[p_name] = param.get("default")

        #any used entities are recorded
        compiled_accounts.add(acct_obj["id"])
        compiled_brokers.add(brk_obj["id"])
        for f in feed_objs:
            compiled_feeds.add(f["id"])

        #create the sim object
        sim_instance = {
            "id": sim_instance_id,
            "strategy":strat_key,
            "account_link": acct_obj["id"],
            "broker_link": brk_obj["id"],
            "feeds": [f["id"] for f in feed_objs],
            "parameters": extracted_params,
            "run_all_by_default": bool(run_default)
        }
        compiled_simulations.append(sim_instance)

    #build the full JSON tree
    batch_id = dpg.get_value("ui_batch_id") or "batch_001"
    batch_notes = dpg.get_value("ui_batch_notes") or "No Notes"

    batch_config = {
        "simulation_metadata": {
            "batch_id": batch_id,
            "notes": batch_notes,
            "created_at": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        },
        "account": [a for a in core.state["registered_accounts"]["account"] if a["id"] in compiled_accounts],
        "broker": list(used_broker_objs.values()),
        "data_feeds": [f for f in core.state["registered_feeds"]["data_feeds"] if f["id"] in compiled_feeds],
        "simulations": compiled_simulations
    }
    print(batch_config)
    return batch_config, None


# =============================================================
# BATCH AND CONFIG MANAGEMENT
# =============================================================

#add entity to node bench in view 3
#this is the callback behind every "+ ..." button in the left sidebar(see
#refresh_registry_sidepane right below) - user_data is a 2-item tuple
#("ACCOUNT"/"BROKER"/"FEED"/"STRATEGY", the actual data dict), packed
#together when each button is created, and unpacked back apart here
def add_entity_to_batch(sender, app_data, user_data):
    entity_type, entity_data = user_data


    if entity_type == "ACCOUNT":
        spawn_account_node(entity_data)
    elif entity_type == "BROKER":
        spawn_broker_node(entity_data)
    elif entity_type == "FEED":
        spawn_feed_node(entity_data)
    elif entity_type == "STRATEGY":
        spawn_strategy_node(entity_data)
        
    print(f"[UI] Spawned {entity_type} Node: {entity_data['id']}")

#refresh sidepane in view 3
#rebuilds the "Node Library" sidebar buttons from scratch every time it's
#called(delete everything currently there, then re-add one button per
#registered account/broker/feed/strategy) - simpler to reason about than
#trying to add/remove individual buttons to match whatever changed, at the
#cost of a bit of redundant work on every refresh
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
                label=f"+ {st['display_name']}", 
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
                #if feedNum is positive the number of feeds is strict
                feedNum = strat.get("feedNum", 1)
                if feedNum > 0:
                    dpg.add_text(feedNum)
                #if the feedNum is negative, that means atleast the abs value of the feedNum
                else:
                    dpg.add_text(f" >= {abs(feedNum)}")
                
                #format parameters
                #str.join(list) glues a list of strings together with the
                #given separator between each one - here turning a list
                #comprehension of formatted "name (type=default)" strings
                #into one single ", "-separated display string
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
            #"with dpg.table_row(...):" adds one new row to the table, and
            #every dpg.add_text/add_button underneath becomes one cell in
            #that row, left to right - matching the add_table_column calls
            #that defined this table's columns further down in VIEWPORT 2
            with dpg.table_row(parent="table_accounts"):
                dpg.add_text(acct["id"])
                dpg.add_text(f"${acct['initial_balance']}")
                dpg.add_text(str(acct["reset"]))
                with dpg.group(horizontal=True):
                    dpg.add_button(label="Edit", callback=spawn_create_account_modal, user_data=acct)
                    #"lambda s, a, u: ..." spells out all three callback
                    #parameters(sender, app_data, user_data) explicitly,
                    #even though only u(user_data) is actually used - this
                    #is required because dearpygui always calls a callback
                    #with all three positional arguments, so a lambda
                    #accepting fewer than three would error out when clicked
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

#"test" here is just a generic [filepath, batch_config] pair(see the
#confirm_callback wiring in export_batch_json below) - not a unit test,
#the name is just a leftover placeholder from development
def export_confirm(test):
    try:
        dpg.split_frame()
        logger_id = spawn_message_modal("Logger", f"Exporting Batch")
        with open(test[0], "w") as f:
            json.dump(test[1], f, indent=4)

        #refresh registries(such as state)
        #hasattr(object, "name") checks whether an object has an
        #attribute/method with that name WITHOUT calling it - a defensive
        #check here in case core.py's module-level code ever failed to
        #fully load(so core.reload_registers might not exist), rather than
        #assuming it's always safe to call
        if hasattr(core, "reload_registers"):
            core.reload_registers()


        refresh_all_ui()
        close_modal(logger_id)
        spawn_message_modal("Batch Exported", f"Batch configuration successfully saved to:\n{test[0]}", need_ok = True)
    except Exception as e:
        spawn_message_modal("Export Error", f"Failed to save batch JSON:\n{repr(e)}", is_error=True)

def export_batch_json():
    #get the batch and error message if one is present
    dpg.split_frame()
    logger_id = spawn_message_modal("Logger", f"Parsing Workbench")

    batch_config, error_msg = parse_workbench_canvas()

    if error_msg:
        close_modal(logger_id)
        spawn_message_modal("Validation Error", error_msg, is_error=True)
        return

    close_modal(logger_id)
    dpg.split_frame()
    logger_id = spawn_message_modal("Logger", f"Checking Directory")

    batch_id = batch_config["simulation_metadata"]["batch_id"]
    batch_dir = os.path.join("..", "config/batchConfig")
    os.makedirs(batch_dir, exist_ok=True)
    filepath = os.path.join(batch_dir, f"{batch_id}.json")
    test = [filepath, batch_config]
    #check if the file exists
    if os.path.exists(filepath):
        close_modal(logger_id)
        spawn_message_modal(
            "Batch ID Conflict", #create a new message with a callback for the exporting
            f"A batch configuration with ID '{batch_id}' already exists at:\n{filepath}\n\n Proceed with download?",confirm_callback = export_confirm, callback_data = test
        )
        return
    else:
        close_modal(logger_id)
        export_confirm(test)

#this is where the python gui actually reaches the compiled c++ engine -
#and it does so with NO direct function calls or language bindings at all.
#it shells out to two separate external programs via subprocess.run(a
#function that launches another program and waits for it to finish,
#exactly like typing a command in a terminal yourself): first "cmake
#--build" to rebuild runny from the latest source, then runny itself with
#the batch json's path as its one command-line argument(the same argv[1]
#main.cpp reads). all python and c++ ever exchange is that file path in,
#and whatever files runny writes to output/ once it's done - see core.py's
#load_simulation_results for the other half of that contract
def run_simulation():

    batchFile = dpg.get_value("ui_batch_id") or "batch_001"
    exec_dir = os.path.abspath(os.path.join("..", "config", "batchConfig"))
    os.makedirs(exec_dir, exist_ok=True)
    temp_filepath = os.path.join(exec_dir, f"{batchFile}.json")

    if not os.path.exists(temp_filepath):
        spawn_message_modal(
            "Batch File", #create a new message with a callback for the exporting
            f"Batch file {temp_filepath} not found. Save batch into json?",confirm_callback = export_batch_json
        )
        return
    batch_config, _ = parse_workbench_canvas()
    def buildFile():
        build_path = os.path.abspath(os.path.join("..", "build"))
        try:
            print("Building project...")
            #the list ["cmake", "--build", "../build"] is the command and
            #its arguments, spelled out as separate list items rather than
            #one shell string - this avoids shell-injection risk entirely,
            #since nothing here ever gets interpreted by a shell.
            #check=True makes subprocess.run RAISE an exception
            #(CalledProcessError, caught below) if the command exits with a
            #non-zero(failure) status, instead of silently continuing
            subprocess.run(
                ["cmake", "--build", "../build"],
                check=True
            )
            print("Build completed successfully!")


        except subprocess.CalledProcessError as e:
            #"file=sys.stderr" sends this print to the error stream instead
            #of standard output - the usual convention for error messages,
            #keeping them separately identifiable from normal program output
            print(f"Error during CMake execution: {e}", file=sys.stderr)
            spawn_message_modal("Build Error", f"Error during CMake execution: {e}", is_error = True)
            return
    #execute runny
    #this whole function runs on a SEPARATE thread(see threading.Thread
    #below) so the actual backtest - which can take a while - doesn't
    #freeze the entire gui while it runs. capture_output=True grabs the
    #engine's stdout/stderr as text instead of letting it print straight to
    #the terminal, and text=True decodes it as a normal string rather than
    #raw bytes
    def engine_thread_task():
        print(f"[Engine] Starting simulation using: {temp_filepath}")
        try:
            #pass the batch file as a command line argument
            dpg.split_frame()
            logger_id = spawn_message_modal("Logger", f"Running Simulation")
            process = subprocess.run([exe_path, temp_filepath], capture_output=True, text=True)

            #a process's returncode is 0 on success, nonzero on failure -
            #the same exit-code convention main.cpp's own "return 0;"/
            #"return 1;" statements produce
            if process.returncode == 0:
                print(f"[Engine Output]\n{process.stdout}")
                # Note: If you want to spawn a modal here, it must be thread-safe or queued
                #heads up: this comment is a real, acknowledged risk, not
                #just a note - dearpygui's ui functions generally aren't
                #meant to be called from any thread other than the main
                #one, but spawn_message_modal/close_modal ARE being called
                #from this background thread right here anyway. it works
                #in practice most of the time, but isn't guaranteed safe -
                #a proper fix would queue these ui updates to run on the
                #main thread instead of calling them directly from here
                print("[Engine] Simulation completed successfully!")
                close_modal(logger_id)
                dpg.split_frame()
                logger_id = spawn_message_modal("Logger", f"Simulation completed successfully!", need_ok = True)
                open_results_dashboard(batch_config)
            else:
                print(f"[Engine Error]\n{process.stderr}")
                print("[Engine] Simulation failed.")
                close_modal(logger_id)
                dpg.split_frame()
                spawn_message_modal("Execution Error", f"Simulation failed", is_error = True)
        except Exception as e:
            print(f"[Engine Crash] {e}")
            close_modal(logger_id)
            dpg.split_frame()
            spawn_message_modal("Execution Error", f"Engine crash {e}", is_error = True)
    #locate runny executable
    exe_path = os.path.abspath(os.path.join("..", "build", "runny"))
    buildFile()

    #threading.Thread(target=fn) creates a new thread that will run fn();
    #daemon=True marks it as a background thread that python is allowed to
    #kill automatically when the main program exits, so a still-running
    #simulation can't keep the whole gui process alive after you close the
    #window. .start() actually begins running it - engine_thread_task
    #starts executing concurrently with the rest of the gui from this
    #point on, not immediately when the Thread object was created
    engine_thread = threading.Thread(target=engine_thread_task, daemon=True)
    engine_thread.start()


def refresh_all_ui():
    refresh_registry_sidepane()
    refresh_config_manager_tables()

#themes for entire window
#"with dpg.theme() as global_theme:" both creates a theme item AND assigns
#it to the python variable global_theme in one line("as" works the same
#way here as it does on a "with open(...) as f:" file handle) - style
#values set inside theme_component(dpg.mvAll) apply to every widget in the
#whole app, since mvAll means "all widget types", rather than needing to
#style each button/window individually
with dpg.theme() as global_theme:
    with dpg.theme_component(dpg.mvAll):
        dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 14, 14, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_FramePadding, 6, 6, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing, 8, 8, category=dpg.mvThemeCat_Core)
#bind_theme actually applies the theme built above to the whole application
dpg.bind_theme(global_theme)

#keyboard configs
#a handler_registry is dearpygui's way of listening for input that isn't
#tied to one specific widget(a key press anywhere, rather than a click on
#one particular button) - add_key_press_handler(mvKey_Back, ...) wires the
#backspace/delete key up to delete_selected_canvas_items globally
def setup_canvas_keyboard_handlers():
    #allows deleting hotkey
    if not dpg.does_item_exist("canvas_handler_registry"):
        with dpg.handler_registry(tag="canvas_handler_registry"):
            dpg.add_key_press_handler(dpg.mvKey_Back, callback=delete_selected_canvas_items)


def delete_selected_canvas_items():
    #allows to delete visually selected items
    if not dpg.does_item_exist("node_editor_canvas"):
        return

    #the node editor tracks its own "currently selected"(clicked/
    #ctrl-clicked) nodes and wires internally - these two calls just ask it
    #for that current selection, rather than this code needing to track
    #selection state itself
    selected_nodes = dpg.get_selected_nodes("node_editor_canvas")
    selected_links = dpg.get_selected_links("node_editor_canvas")
    
    # Delete selected wires
    for link in selected_links:
        dpg.delete_item(link)
        
    # Delete selected node containers
    for node in selected_nodes:
        dpg.delete_item(node)

#target directory for file selection
default_batch_dir = os.path.abspath(os.path.join("..", "config/batchConfig"))

os.makedirs(default_batch_dir, exist_ok=True)

#dpg.file_dialog's callback receives an app_data DICT(unlike most other
#widgets, where app_data is just the new value) with details about
#whatever the user picked - "file_path_name" is the full path to the
#selected file
def on_batch_file_selected(sender, app_data):
    selected_path = app_data.get("file_path_name")
    if selected_path:
        load_batch_file_to_workbench(selected_path)

#batch file dialog
#this file_dialog is built ONCE here at import time(tag="batch_file_dialog",
#show=False) rather than freshly created every time it's needed - other
#code just calls dpg.show_item("batch_file_dialog") to reveal this same,
#already-built dialog(see the landing hub button further down)
with dpg.file_dialog(directory_selector=False,show=False,callback=on_batch_file_selected,tag="batch_file_dialog",width=700,height=400, default_path=default_batch_dir):
    dpg.add_file_extension(".json", color=[0, 255, 0, 255])

def open_results_from_batch_file(sender, app_data):
    #whenever user selects a batch config for the results
    selected_file = app_data.get("file_path_name")
    if not selected_file or not os.path.exists(selected_file):
        return

    try:
        with open(selected_file, "r") as f:
            batch_config = json.load(f)
            
        #open results dashboard
        open_results_dashboard(batch_config)
    except Exception as e:
        spawn_message_modal("File Read Error", f"Could not load batch JSON:\n{e}", is_error=True)


def create_results_file_dialog():
    #spawn file dialog for results
    default_batch_dir = os.path.abspath(os.path.join("..", "config", "batchConfig"))
    os.makedirs(default_batch_dir, exist_ok=True)
    tag_name = "open_results_file_dialog"
    if dpg.does_item_exist(tag_name):
        dpg.show_item(tag_name)
        return

    with dpg.file_dialog(
        directory_selector=False,
        show=True,
        callback=open_results_from_batch_file,
        tag=tag_name,
        width=700,
        height=400,
        default_path=default_batch_dir
    ):
        dpg.add_file_extension(".json", color=[0, 255, 0, 255])

#NOTE in dearpygui, windows are the main canvas, while viewports are the individual sections you see

# =============================================================
# VEIEWPORTS
# =============================================================
#everything from here down to EXECUTION at the very bottom of the file runs
#immediately at import time(these "with dpg.window(...):" blocks are not
#inside any function), building the STATIC shell of all four pages once -
#the actual DYNAMIC content(table rows, sidebar buttons, node canvas
#contents) is deliberately left empty here and filled in later by the
#refresh_*() functions defined earlier in this file, called once real data
#is available(see refresh_all_ui() near the very bottom)
#
#"width=-1"/"height=-1" is a dearpygui convention meaning "stretch to fill
#whatever space is left in the parent", rather than a literal negative
#pixel size - used throughout these viewports so panels resize sensibly
#instead of needing a hardcoded pixel size that would only look right at
#one specific window size

# ================================
# VIEWPORT 1: LANDING PAGE
# ================================
with dpg.window(tag="landing_hub_window", no_move=True, no_resize=True, no_title_bar=True, show=True):
    dpg.add_text("Backtesting Engine By Vivek Venigalla", color=[100, 200, 255])
    dpg.add_separator()
    dpg.add_spacer(height=10)
    with dpg.group(horizontal=True):
        #child_window creates a scrollable sub-panel inside the current
        #window - used throughout this file to split one page into
        #independently-sized/scrolled regions(here, a fixed-width left
        #action panel next to a flexible-width right activity panel)
        #left panel containg quick actions such as creating a batch or manageing global config
        with dpg.child_window(width=320, height=-1):
            dpg.add_button(label="Open Batch File for Editing", width = -1, height = 50, callback=lambda: dpg.show_item("batch_file_dialog"))
            dpg.add_spacer(height=10)
            dpg.add_button(label="Open Batch Workbench Window", width=-1, height=50, callback=lambda: route_to_view("workbench_window"))
            dpg.add_spacer(height=10)
            dpg.add_button(label="Manage Global Infrastructure", width=-1, height=50, callback=lambda: route_to_view("config_manager_window"))
            dpg.add_spacer(height = 10)
            dpg.add_button(label="View Batch Results", width=-1, height=50, callback=create_results_file_dialog)
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
    
    #tab_bar/tab give you the familiar clickable-tabs ui(only one tab's
    #contents visible at a time) - each dpg.table below is created empty
    #(just column headers via add_table_column) and gets its actual rows
    #filled in later by refresh_config_manager_tables()
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
            #two string literals sitting next to each other with nothing
            #but whitespace between them get automatically joined into one
            #single string at parse time - this is just a plain string,
            #split across two lines purely for readability in the source,
            #not two separate arguments
            dpg.add_text(
                "Strategy Blueprints(Read Only). These correspond to C++ files in the /strategies folder."
                "Parameters can be customized when added to the Node Workbench.",
                color=[180, 180, 180],
                wrap=600
            )
            dpg.add_spacer(height=5)
            with dpg.table(tag="table_strategies", header_row=True, borders_innerH=True, borders_outerH=True, borders_innerV=True, borders_outerV=True):
                dpg.add_table_column(label="Strategy ID", width_fixed=True, init_width_or_weight=75)
                dpg.add_table_column(label="Display Name", width_fixed=True, init_width_or_weight=200)
                dpg.add_table_column(label="Description")
                dpg.add_table_column(label="Number of Feeds", width_fixed=True, init_width_or_weight=50)
                dpg.add_table_column(label="Default Parameters")

# ==============================
# VIEWPORT 3: BATCH MANEGEMENT
# ==============================
with dpg.window(tag="workbench_window", no_move=True, no_resize=True, no_title_bar=True, show=False):
    with dpg.group(horizontal=True):
        dpg.add_button(label="Back to Hub", callback=lambda: route_to_view("landing_hub_window"))
        dpg.add_text("WORKBENCH PIPELINE MANAGEMENT", color=[100, 200, 255])
        dpg.add_spacer(width=20)
        dpg.add_button(label="Save & Compile Batch JSON", callback=export_batch_json)
        dpg.add_button(label="Run Simulation", callback=run_simulation)
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

            #collapsing_header is the expandable/collapsible section widget
            #(click the label to fold it away) - each one here wraps an
            #EMPTY group with a fixed tag("reg_accounts_container" etc);
            #"pass" is python's "do nothing" placeholder statement, needed
            #here only because a "with" block can't have an empty body.
            #these groups start empty on purpose and get their actual
            #button contents added later by refresh_registry_sidepane(),
            #which looks them up by these exact same tags
            with dpg.collapsing_header(label="Accounts"):
                with dpg.group(tag="reg_accounts_container"): pass

            with dpg.collapsing_header(label="Brokers"):
                with dpg.group(tag="reg_brokers_container"): pass

            with dpg.collapsing_header(label="Data Feeds"):
                with dpg.group(tag="reg_feeds_container"): pass

            with dpg.collapsing_header(label="Strategies"):
                with dpg.group(tag="reg_strategies_container"): pass

        #node editor
        #this is the actual node-editor widget the spawn_*_node functions
        #earlier in this file all target via parent="node_editor_canvas" -
        #its own callback/delink_callback are what dearpygui calls
        #automatically when the user draws or removes a wire, wiring
        #straight into on_node_link_created/on_node_link_deleted
        with dpg.child_window(width=-1, height=-1):
            with dpg.node_editor(tag="node_editor_canvas",callback=on_node_link_created, delink_callback=on_node_link_deleted):
                pass

# ==============================
# VIEWPORT 4: BATCH RESULTS
# ==============================
#unlike viewports 1-3, this one isn't a static window built once at import
#time - open_results_dashboard() further down builds this whole window
#fresh, from scratch, every time you actually want to look at a batch's
#results, since which simulations exist to show only becomes known once a
#specific batch's output has been loaded

# Global state to hold currently loaded batch results
#module-level(not inside a function) variables like these two are how this
#file's callbacks share state - render_left_metrics_pane, the strategy
#dropdown's callback, and open_results_dashboard all read/write the same
#CURRENT_BATCH_RESULTS/ACTIVE_SIM_ID rather than passing this data around
#as function arguments everywhere
CURRENT_BATCH_RESULTS = {}
ACTIVE_SIM_ID = None


def render_left_metrics_pane(sim_data):
    #refresh metric valus
    metrics = sim_data.get("metrics", {})
    trade_recs = metrics.get("Trade_records", {})
    cagr = metrics.get("CAGR", 0.0)
    tot_returns = metrics.get("totalReturns", 0.0)
    num_trades = trade_recs.get("Number_of_trades", 0)
    succ_trades = trade_recs.get("Successful_trades", 0)
    unsucc_trades = trade_recs.get("Unsuccessful_trades", 0)

    #heads up: this recomputes win rate by hand from the raw trade counts -
    #metricData.json now actually includes a ready-made "Win_rate" field
    #directly(see Logger::exportJSON on the c++ side), so this could be
    #simplified to metrics.get("Trade_records", {}).get("Win_rate", 0.0)
    #instead. this python code predates that field being added
    win_rate = (succ_trades / num_trades * 100) if num_trades > 0 else 0.0

    # Color helpers (Green for positive, Red for negative)
    ret_color = [0, 255, 127] if tot_returns >= 0 else [255, 80, 80]

    #":.2f" is a format spec meaning "fixed-point, 2 digits after the
    #decimal" - so 5.126 becomes the string "5.13", not the full raw float
    dpg.set_value("ui_val_cagr", f"{cagr:.2f}%")
    dpg.set_value("ui_val_tot_returns", f"{tot_returns:.2f}%")
    dpg.configure_item("ui_val_tot_returns", color=ret_color)
    
    dpg.set_value("ui_val_num_trades", str(num_trades))
    dpg.set_value("ui_val_succ_trades", str(succ_trades))
    dpg.set_value("ui_val_unsucc_trades", str(unsucc_trades))
    dpg.set_value("ui_val_win_rate", f"{win_rate:.1f}%")

#heads up: this re-import is a vestigial copy-paste artifact - "import
#dearpygui.dearpygui as dpg" already happened once at the very top of this
#file, and python simply reuses the same already-loaded module the second
#time rather than re-importing anything, so this line is harmless but
#serves no purpose here
import dearpygui.dearpygui as dpg

#dpg.plot/add_plot_axis/add_line_series are dearpygui's own native charting
#widgets - a genuinely different rendering path from the matplotlib-based
#plotter.py script, but showing the same kind of equity-curve/drawdown data
def render_right_charts_pane(sim_data):
    #render dearpygui plots
    timeseries = sim_data.get("timeseries", {})
    equities = timeseries.get("equities", [])
    balances = timeseries.get("balances", [])
    drawdowns = timeseries.get("drawdowns", [])

    #genereate x-axis indices
    #range(n) generates the whole numbers 0,1,2,...,n-1 lazily(on demand);
    #wrapping it in list() forces all of them out into a real list right
    #away - here just used as plain "bar number" x-axis values, one per
    #equity point, since there's no per-bar date array plumbed through to
    #this chart
    x_indices = list(range(len(equities)))

    #clear previous charts
    if dpg.does_item_exist("plot_render_group"):
        dpg.delete_item("plot_render_group", children_only=True)

    #push new plot elements into plot group
    with dpg.group(parent="plot_render_group"):
        if not equities:
            dpg.add_text("No time-series bar data available for this simulation.", color=[255, 100, 100])
            return

        #equity and balance curve
        with dpg.plot(label="Portfolio Value Over Time", height=350, width=-1):
            dpg.add_plot_legend()
            
            #axes setup
            dpg.add_plot_axis(dpg.mvXAxis, label="Time (Bars)")
            y_axis = dpg.add_plot_axis(dpg.mvYAxis, label="USD ($)")

            #series rendering
            dpg.add_line_series(x_indices, equities, label="Equity", parent=y_axis)
            dpg.add_line_series(x_indices, balances, label="Cash Balance", parent=y_axis)

        dpg.add_spacer(height=10)

        #drawdown
        with dpg.plot(label="Underwater Drawdown (%)", height=180, width=-1):
            dpg.add_plot_axis(dpg.mvXAxis, label="Time (Bars)")
            dd_y_axis = dpg.add_plot_axis(dpg.mvYAxis, label="Drawdown %")

            #rendered in red
            dpg.add_line_series(x_indices, drawdowns, label="Drawdown", parent=dd_y_axis)

def on_strategy_dropdown_changed(sender, app_data, user_data):
    #dropdown for a different strategy
    #by default, assigning to a name inside a function creates a NEW local
    #variable, even if a module-level(global) variable with that same name
    #already exists - "global ACTIVE_SIM_ID" tells python "no, when this
    #function assigns to ACTIVE_SIM_ID, modify the real module-level one
    #from VIEWPORT 4's setup, don't shadow it with a local". reading a
    #global's value(like CURRENT_BATCH_RESULTS just below) never needs this
    #declaration, only reassigning it does
    global ACTIVE_SIM_ID
    ACTIVE_SIM_ID = app_data
    if ACTIVE_SIM_ID in CURRENT_BATCH_RESULTS:
        sim_data = CURRENT_BATCH_RESULTS[ACTIVE_SIM_ID]
        render_left_metrics_pane(sim_data)
        #chart updates here
        render_right_charts_pane(sim_data)


#builds the whole VIEWPORT 4 window fresh(unlike viewports 1-3, this one
#doesn't exist until you actually have results to show) - called either
#right after a simulation finishes(from run_simulation's engine_thread_task)
#or when you open an existing batch's results from the landing hub
def open_results_dashboard(batch_config):
    #main window to open
    #two names after "global", comma-separated - both CURRENT_BATCH_RESULTS
    #and ACTIVE_SIM_ID get reassigned in this function, so both need
    #declaring(see on_strategy_dropdown_changed above for what global does)
    global CURRENT_BATCH_RESULTS, ACTIVE_SIM_ID

    #load results
    CURRENT_BATCH_RESULTS = core.load_batch_results(batch_config)
    #this triple-quoted block isn't disabled code(compare to
    #csv_download.py's use of the same syntax) - it's purely left here as
    #inline documentation, reminding a reader what shape each entry in
    #CURRENT_BATCH_RESULTS actually has, matching the "results" dict
    #core.load_simulation_results builds and returns
    '''results = {
        "sim_id": sim_id,
        "metrics": {},
        "timeseries": {
            "dates": [],
            "balances": [],
            "equities": [],
            "drawdowns": [],
            "prices": []
        },
        "trades": []
    }'''

    if not CURRENT_BATCH_RESULTS:
        spawn_message_modal("Results Error", "No simulation output data found", is_error=True)
        return

    #get all the sim ids with the keys of results
    sim_ids = list(CURRENT_BATCH_RESULTS.keys())
    ACTIVE_SIM_ID = sim_ids[0]

    #destroy window if it already exists
    if dpg.does_item_exist("results_dashboard_window"):
        dpg.delete_item("results_dashboard_window")

    #spawn main window
    #on_close is a special dearpygui callback that fires when the user
    #clicks this specific window's own close button(the x in its title
    #bar) - wired here to actually delete the window item, otherwise
    #closing it would just hide it and leave it sitting around in memory
    with dpg.window(
        label="Simulation Results",
        tag="results_dashboard_window",
        width=1100,
        height=700,
        pos=[50, 50],
        on_close=lambda: dpg.delete_item("results_dashboard_window")
    ):
        #header bar
        with dpg.group(horizontal=True):
            batch_id = batch_config.get("simulation_metadata", {}).get("batch_id", "Unknown")
            dpg.add_text(f"Batch ID: {batch_id}", color=[0, 200, 255])
            dpg.add_spacer(width=30)
            dpg.add_text("Select Strategy:")
            #add_combo is a dropdown selector - items is the list of
            #choices shown, and picking one fires on_strategy_dropdown_
            #changed with the picked string as app_data
            dpg.add_combo(
                items=sim_ids,
                default_value=ACTIVE_SIM_ID,
                callback=on_strategy_dropdown_changed,
                width=200
            )

        dpg.add_separator()

        #main two pane layout
        with dpg.group(horizontal=True):
            #left pane, metrics (1/3 width ~320)
            with dpg.child_window(width=320, height=580, border=True):
                dpg.add_text("Performance Metrics", color=[255, 200, 100])
                dpg.add_separator()
                
                with dpg.group(horizontal=True):
                    dpg.add_text("Total Returns:")
                    dpg.add_text("0.0%", tag="ui_val_tot_returns")

                with dpg.group(horizontal=True):
                    dpg.add_text("CAGR:")
                    dpg.add_text("0.0%", tag="ui_val_cagr")

                dpg.add_spacer(height=15)
                dpg.add_text("Trade Statistics", color=[255, 200, 100])
                dpg.add_separator()

                with dpg.group(horizontal=True):
                    dpg.add_text("Total Trades:")
                    dpg.add_text("0", tag="ui_val_num_trades")

                with dpg.group(horizontal=True):
                    dpg.add_text("Win Rate:")
                    dpg.add_text("0.0%", tag="ui_val_win_rate")

                with dpg.group(horizontal=True):
                    dpg.add_text("Successful Trades:")
                    dpg.add_text("0", tag="ui_val_succ_trades", color=[0, 255, 127])

                with dpg.group(horizontal=True):
                    dpg.add_text("Unsuccessful Trades:")
                    dpg.add_text("0", tag="ui_val_unsucc_trades", color=[255, 80, 80])

            #right pane, chart container
            with dpg.child_window(tag="right_plot_container", width=-1, height=580, border=True):
                dpg.add_text("Timeseries Performance Plots", color=[255, 200, 100])
                dpg.add_separator()

                #placeholder for charts
                with dpg.group(tag="plot_render_group"):
                    dpg.add_text("Charts will render here...", color=[150, 150, 150])

    #every "0.0%"/"0"-tagged text item above(ui_val_tot_returns,
    #ui_val_cagr, etc) is just a static placeholder - render_left_metrics_
    #pane/render_right_charts_pane below immediately overwrite them with
    #the real numbers via dpg.set_value(tag, ...), the instant this window
    #finishes being built
    #render initial metrics
    first_sim_data = CURRENT_BATCH_RESULTS[ACTIVE_SIM_ID]
    render_left_metrics_pane(first_sim_data)
    render_right_charts_pane(first_sim_data)

# =============================================================
# EXECUTION
# =============================================================
#everything above this point only BUILT the ui tree - nothing is actually
#visible or interactive yet until these last few lines run. this is the
#standard dearpygui startup sequence: create_viewport makes the actual os
#window, setup_dearpygui finalizes internal state, and show_viewport makes
#it visible on screen
dpg.create_viewport(title='Backtesting Engine', width=1300, height=840)
dpg.setup_dearpygui()
dpg.show_viewport()

dpg.set_viewport_resize_callback(resize_windows_handler)
resize_windows_handler()

# Render side-pane contents from core.state safely
#this is the moment the static shell built by the VIEWPORT blocks above
#finally gets populated with real data - everything before this line
#exists, but the tables/sidebar are still empty until refresh_all_ui()
#actually runs
refresh_all_ui()
setup_canvas_keyboard_handlers()

#dpg.start_dearpygui() hands control over to dearpygui's own event loop -
#this call BLOCKS(doesn't return) for as long as the window stays open,
#continuously redrawing frames and dispatching clicks/keypresses to
#whatever callbacks were registered throughout this whole file. once the
#user closes the window, start_dearpygui finally returns and
#destroy_context cleans up dearpygui's internal state before the program
#actually exits
dpg.start_dearpygui()
dpg.destroy_context()