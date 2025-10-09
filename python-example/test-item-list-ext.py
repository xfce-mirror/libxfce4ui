#!/usr/bin/env python3
import gi.repository
gi.require_version('GioUnix', '2.0')
gi.require_version('Libxfce4ui', '2.0')
gi.require_version('Gtk', '3.0')
from gi.repository import Libxfce4ui as Xfce
from gi.repository import Gtk
from gi.repository import GObject
from gi.repository import Gio

from dataclasses import dataclass
from enum import IntEnum, auto
import datetime

@dataclass
class Todo:
  name: str
  created_at: datetime.datetime
  done: bool
  important: bool

  def toggle_important(self):
    self.important = not self.important

class TodoModel(Xfce.ItemListModel):
  class Column(IntEnum):
    TIME = Xfce.ItemListModelColumn.USER
    N_COLUMNS = auto()
  
  def __init__(self):
    Xfce.ItemListModel.__init__(self)
    self.todos = []

  def do_get_n_columns(self):
    return TodoModel.Column.N_COLUMNS

  def do_get_list_column_type(self, index):
    if index == TodoModel.Column.TIME:
      return GObject.TYPE_STRING
    else:
      return Xfce.ItemListModel.do_get_list_column_type(self, index)

  def do_get_list_flags(self):
    return (Xfce.ItemListModelFlags.REORDERABLE
      | Xfce.ItemListModelFlags.EDITABLE
      | Xfce.ItemListModelFlags.ADDABLE
      | Xfce.ItemListModelFlags.REMOVABLE)

  def do_get_n_items(self):
    return len(self.todos)

  def do_get_item_value(self, index, column):
    todo = self.todos[index]
    if column == Xfce.ItemListModelColumn.ACTIVE:
      return todo.done
    elif column == Xfce.ItemListModelColumn.ACTIVABLE:
      return True
    elif column == Xfce.ItemListModelColumn.ICON:
      if todo.important:
        return Gio.ThemedIcon.new('dialog-warning')
    elif column == Xfce.ItemListModelColumn.NAME:
      return todo.name
    elif column == Xfce.ItemListModelColumn.EDITABLE:
      return True
    elif column == Xfce.ItemListModelColumn.REMOVABLE:
      return True
    elif column == TodoModel.Column.TIME:
      return todo.created_at.strftime('%a %d %b %Y, %I:%M%p')

  def do_move(self, source_index, dest_index):
    tmp = self.todos[source_index]
    del self.todos[source_index]
    self.todos.insert(dest_index, tmp)

  def do_set_activity(self, index, value):
    self.todos[index].done = value

  def do_edit(self, index):
    if '*' not in self.todos[index].name:
      self.todos[index].name = self.todos[index].name + '*'

  def do_remove(self, index):
    del self.todos[index]
    return True

  def do_add(self):
    self.todos.append(Todo('Todo #' + str(len(self.todos) + 1), datetime.datetime.now(), False, False))
    return True

class Window(Gtk.Window):
  def __init__(self):
    super().__init__()
    self.set_size_request(640, 480)
    
    self.model = TodoModel()
    
    action_group = Gio.SimpleActionGroup()
    mark_action = Gio.SimpleAction.new('mark-important', None)
    mark_action.connect('activate', self.mark_important)
    action_group.add_action(mark_action)
    self.insert_action_group('todo', action_group)
    
    self.view = Xfce.ItemListView(model=self.model)
    self.add(self.view)
    
    menu_item = Gio.MenuItem.new('Mark as important', 'todo.mark-important')
    menu_item.set_attribute_value('icon', Gio.ThemedIcon.new('dialog-warning').serialize())
    self.view.get_menu().append_item(menu_item)

    tree_view = self.view.get_tree_view()
    tree_view.set_headers_visible(True)
    tree_view.append_column(Gtk.TreeViewColumn('created at', Gtk.CellRendererText(), text=TodoModel.Column.TIME, sensitive=Xfce.ItemListModelColumn.ACTIVE))

  def mark_important(self, action, data):
    tree_view = self.view.get_tree_view()
    model, model_iter = tree_view.get_selection().get_selected()
    if model_iter is not None:
      model.todos[model.get_index(model_iter)].toggle_important()
      model.changed()

win = Window()
win.connect("destroy", Gtk.main_quit)
win.show_all()
Gtk.main()
