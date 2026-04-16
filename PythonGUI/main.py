import time

import serial
from kivy.uix import switch
from plyer import filechooser
from kivy.app import App
from kivy.app import Builder
from kivy.properties import StringProperty, ListProperty, ObjectProperty
from kivy.clock import Clock
from kivy.core.window import Window
from kivy.uix.textinput import TextInput


PORT = 'COM5'


class CNCApp(App):
    status_text = StringProperty()
    coords_text = StringProperty()
    drill_pos = ListProperty([0,0])
    rect_size = ListProperty([700, 410])
    rect_pos = ListProperty([300, 150])

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.gcode_lines = None
        self.ser = serial.Serial(port=PORT, baudrate=115200, timeout=0)
        self.coords = [0,0,0]
        self.microsteps = 1
        self.drill_speed = 0
        self.mode_manual = True
        self.current_gcode = []
        self.gcode_done = False
        self.single_input = False
        self._keyboard = Window.request_keyboard(self._keyboard_closed, self.root)
        self._keyboard.bind(on_key_down=self.on_key_down)
        self.ready_to_send = False
        Clock.schedule_interval(self.read_serial, 0.05)


    def on_stop(self):
        self.ser.close()

    def _keyboard_closed(self):
        self._keyboard.unbind(on_key_down=self.on_key_down)
        self._keyboard = None

    def build(self):
        self.title = "CNC Control"
        self.root = Builder.load_file("app.kv")
        self.coords_text = "hi"

        return self.root

    def read_serial(self, dt):
        if self.ser.in_waiting:
            lines = self.ser.readlines()
            for line in lines:
                line = line.decode()
                print(line)
                self.status_text = line
                if line.startswith("POS"):
                    parts = line.split(" ")
                    self.coords[0] = int(parts[1][2:])
                    self.coords[1] = int(parts[2][2:])
                    self.coords[2] = int(parts[3][2:])
                    self.set_coords()
                elif line.strip() == "OK":
                    self.ready_to_send = True
                    if self.gcode_done and not self.mode_manual:
                        self.mode_manual = True
                        self.send_serial("ManualMode\r\n")
                    if self.single_input and not self.mode_manual:
                        self.single_input = False
                        self.mode_manual = True
                        self.send_serial("ManualMode\r\n")
            if self.ready_to_send:
                self.ready_to_send = False
                self.send_gcode()

            # line = self.ser.readline().decode()



    def send_serial(self, command):
        self.ser.write(command.encode())
        print(f"SENDING: {command}")

    def set_spindle_speed(self):
        values = self.root.ids.spindle_speed_spinner.values
        new_speed = values.index(self.root.ids.spindle_speed_spinner.text)
        if new_speed > self.drill_speed:
            self.send_serial("+" * (new_speed-self.drill_speed))
        else:
            self.send_serial("-" * (self.drill_speed-new_speed))
        self.drill_speed = new_speed
        print(self.root.ids.spindle_speed_spinner.text)

    def set_microsteps(self,text):
        microstep_speed = self.root.ids.microsteps_spinner.text
        print(text)
        if microstep_speed == "1step/step":
            self.send_serial("0\r\n")
        elif microstep_speed == "2microsteps/step":
            self.send_serial("1\r\n")
        elif microstep_speed == "4microsteps/step":
            self.send_serial("2\r\n")
        elif microstep_speed == "8microsteps/step":
            self.send_serial("3\r\n")
        elif microstep_speed == "16microsteps/step":
            self.send_serial("4\r\n")
        elif microstep_speed == "32microsteps/step":
            self.send_serial("5\r\n")

    def set_coords(self):
        self.coords_text = f"X: {self.coords[0]/50}mm, Y: {self.coords[1]/50}mm, Z: {self.coords[2]/50}mm"
        self.drill_pos = [self.coords[0]*self.rect_size[0]/15000 + self.rect_pos[0]-7.5, self.coords[1]*self.rect_size[1]/7800 + self.rect_pos[1]-7.5]
        # print(self.rect_pos)
        # circle = self.root.ids.ellipse_id
        # rectangle = self.root.ids.cnc_limits
        # circle.center = (self.coords[0]*rectangle.width/300, self.coords[1]*rectangle.height/175)

    def handle_gcode(self):
        filepath = filechooser.open_file(title="Pick a G-Code File...")
        if filepath:
            print(filepath)

            with open(filepath[0], 'r') as infile:
                self.gcode_done = False
                self.mode_manual = False
                self.send_serial('m')
                self.current_gcode = infile.readlines()
                self.ready_to_send = True

    def send_gcode(self):
        if self.current_gcode:
            self.send_serial(f"{self.current_gcode[0].strip().upper()}\r\n")
            print(self.current_gcode)
            del self.current_gcode[0]
            print(self.current_gcode)
            if not self.current_gcode:
                print("G-Code Done")
                self.gcode_done = True

    def handle_manual_gcode(self):
        self.send_serial("m")
        self.mode_manual = False
        self.single_input = True
        self.send_serial(f"{self.root.ids.g_code_input.text}\r\n")

    def handle_calibrate(self):
        self.send_serial("c")

    def handle_go_home(self):
        self.send_serial("m")
        self.mode_manual = False
        self.single_input = True
        self.send_serial("G28\r\n")

    def handle_set_home(self):
        self.send_serial("m")
        self.mode_manual = False
        self.single_input = True
        self.send_serial("G28.1\r\n")

    def on_key_down(self, keyboard, keycode, text, modifiers):
        if keycode[1] in "qweasd=-012345 l":
            self.send_serial(keycode[1])

    def handle_stop(self):
        self.send_serial("l")

class TextField(TextInput):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def _on_textinput_focused(self, instance, value, *largs):
        if not instance.focus: # When not focussed on text input
            app = App.get_running_app()
            app._keyboard = Window.request_keyboard(app._keyboard_closed, self)
            app._keyboard.bind(on_key_down=app.on_key_down)




if __name__ == '__main__':
    CNCApp().run()