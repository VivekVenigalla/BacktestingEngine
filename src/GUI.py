import dearpygui.dearpygui as dpg
from core import state

#setup up rendering
dpg.create_context()

#navigation between screens
def route_to_view(target_window_tag):
    #list of all screens
    all_screens = ["landing_hub_window", "workbench_window"]
    for view in all_screens:
        #if the screen exists, change the view
        if dpg.does_item_exist(view):
            dpg.configure_item(view, show=(view == target_window_tag))

#padding for screen
with dpg.theme() as global_theme:
    with dpg.theme_component(dpg.mvAll):
        dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 14, 14, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_FramePadding, 6, 6, category=dpg.mvThemeCat_Core)
        dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing, 8, 8, category=dpg.mvThemeCat_Core)
dpg.bind_theme(global_theme)

#resize the screen accordingly
def resize_windows_handler():
    """Calculates active OS window bounds and anchors views seamlessly at [0,0]."""
    # Safeguard viewport measurements against initial render edge cases
    w = max(dpg.get_viewport_width() - 16, 400)
    h = max(dpg.get_viewport_height() - 39, 300)
    
    for screen in ["landing_hub_window", "workbench_window"]:
        if dpg.does_item_exist(screen):
            dpg.configure_item(screen, width=w, height=h, pos=[0, 0])


#landing page
with dpg.window(tag="landing_hub_window", no_move=True, no_resize=True, no_title_bar=True, show=True):
    dpg.add_text("BACKTESTING ENGINE BY VIVEK VENIGALLA", color=[100, 200, 255])
    dpg.add_separator()
    #space
    dpg.add_spacer(height=10)
    
    with dpg.group(horizontal=True):
        #side panel for creating new batch
        with dpg.child_window(width=320, height=-1):
            dpg.add_button(
                label=" + Create New Batch", 
                width=-1, 
                height=50, 
                callback=lambda: route_to_view("workbench_window"), 
            )
            
        #previous batch panel
        with dpg.child_window(width=-1, height=-1):
            dpg.add_text("Batches Folder Directory (From batchConfig/)", color=[140, 140, 140])
            dpg.add_separator()
            dpg.add_spacer(height=5)
            
            #iterate over state batches to get ids and last modified time
            if not state["historical_batches"]:
                dpg.add_text("No historical configurations found. Click above to generate.", color=[255, 165, 0])
            else:
                for batch in state["historical_batches"]:
                    dpg.add_text(f"{batch['batch_id']}.json (Modified: {batch['timestamp']})")


#workbech => making of a batch
with dpg.window(tag="workbench_window", no_move=True, no_resize=True, no_title_bar=True, show=False):
    with dpg.group(horizontal=True):
        dpg.add_button(label="Back to Hub", callback=lambda: route_to_view("landing_hub_window"))
        dpg.add_text("WORKBENCH PIPELINE MANAGEMENT", color=[100, 200, 255])
    dpg.add_separator()
    dpg.add_spacer(height=10)
    
    dpg.add_text("Pipeline Workstation Canvas Placeholder - Core components map here next.")

#execution
dpg.create_viewport(title='Backtesting Engine', width=1300, height=840)
dpg.setup_dearpygui()
dpg.show_viewport()


dpg.set_viewport_resize_callback(resize_windows_handler)
resize_windows_handler()

dpg.start_dearpygui()
dpg.destroy_context()