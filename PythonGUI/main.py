import serial
from plyer import filechooser
from kivy.app import App
from kivy.app import Builder
from kivy.properties import StringProperty, ListProperty
from kivy.clock import Clock
from kivy.core.window import Window

PORT = 'COM5'

class CNCApp(App):
    status_text = StringProperty()
    coords_text = StringProperty()
    drill_pos = ListProperty([0,0,0])

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.gcode_lines = None
        self.ser = serial.Serial(port=PORT, baudrate=115200, timeout=0)
        self.coords = [0,0,0]
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
        self._keyboard = Window.request_keyboard(self._keyboard_closed, self.root)
        self._keyboard.bind(on_key_down=self.on_key_down)
        return self.root

    def read_serial(self, dt):
        while self.ser.in_waiting:
            line = self.ser.readline().decode().strip()
            self.status_text = line

    def send_serial(self, command):
        self.ser.write(command.encode())
        print(command)

    def set_spinner_speed(self):
        print(self.root.ids.spindle_speed_spinner.text)

    def set_microsteps(self):
        print(self.root.ids.microsteps_spinner.text)

    def set_coords(self):
        self.coords_text = f"X: {self.coords[0]}mm, Y: {self.coords[1]}mm, Z: {self.coords[2]}mm"
        circle = self.root.ids.drill_circle
        rectangle = self.root.ids.cnc_limits
        circle.center = (self.coords[0]*rectangle.width/300, self.coords[1]*rectangle.height/175)

    def handle_gcode(self):
        filepath = filechooser.open_file(title="Pick a G-Code File...")
        if filepath:
            print(filepath)
            with open(filepath[0], 'r') as infile:
                for line in infile:
                    self.send_serial(f"{line.strip()}\r\n")

    def on_key_down(self, keyboard, keycode, text, modifiers):
        if keycode[1] in "qweasd=-012345":
            self.send_serial(keycode[1])

if __name__ == '__main__':
    CNCApp().run()