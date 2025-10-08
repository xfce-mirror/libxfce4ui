#!/usr/bin/env python3
import gi.repository
gi.require_version('GioUnix', '2.0')
gi.require_version('Libxfce4ui', '2.0')
gi.require_version('Gtk', '3.0')
from gi.repository import Libxfce4ui as Xfce
from gi.repository import Gtk

# Model that controls the order of buttons
class ButtonsModel(Xfce.ItemListModel):
  def __init__(self, box):
    Xfce.ItemListModel.__init__(self)
    self.box = box

  def do_get_list_flags(self):
    return Xfce.ItemListModelFlags.REORDERABLE | Xfce.ItemListModelFlags.EDITABLE

  def do_get_n_items(self):
    return len(self.box.get_children())

  def do_get_item_value(self, index, column):
    child = self.box.get_children()[index]
    if column == Xfce.ItemListModelColumn.COLUMN_ACTIVE:
      return child.get_sensitive()
    elif column == Xfce.ItemListModelColumn.COLUMN_ACTIVABLE:
      return True
    elif column == Xfce.ItemListModelColumn.COLUMN_NAME:
      return child.get_label()
    elif column == Xfce.ItemListModelColumn.COLUMN_ICON:
      return None
    elif column == Xfce.ItemListModelColumn.COLUMN_TOOLTIP:
      return None
    elif column == Xfce.ItemListModelColumn.COLUMN_EDITABLE:
      return True
    elif column == Xfce.ItemListModelColumn.COLUMN_REMOVABLE:
      return True

  def do_move(self, source_index, dest_index):
    self.box.reorder_child(self.box.get_children()[source_index], dest_index)

  def do_set_activity(self, index, value):
    self.box.get_children()[index].set_sensitive(value)

  def do_edit(self, index):
    child = self.box.get_children()[index]
    child.set_label(child.get_label() + '*')

# Let's create an application window
class Window(Gtk.Window):
  def __init__(self):
    super().__init__()

    vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
    self.add(vbox)

    # Let's create a model for buttons and fill it with several buttons
    bbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
    bbox.add(Gtk.Button(label='Cow'))
    bbox.add(Gtk.Button(label='Dog'))
    bbox.add(Gtk.Button(label='Whale'))
    bbox.add(Gtk.Button(label='Sheep'))
    model = ButtonsModel(bbox)

    # Let's create a model viewing widget
    view = Xfce.ItemListView(model=model)
    self.set_size_request(640, 480)

    vbox.add(view)
    vbox.add(bbox)

win = Window()
win.connect("destroy", Gtk.main_quit)
win.show_all()
Gtk.main()
