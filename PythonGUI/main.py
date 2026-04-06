import serial
from kivy.app import App
from kivy.app import Builder
from kivy.uix.button import Button

PORT = "COM5"


class CNCApp(App):
    """App class to control CNC machine"""

    def __init__(self, **kwargs):
        """Construct main app"""
        super().__init__(**kwargs)
    
    def __build__(self):
        """Build Kivy GUI"""
        self.title = "Control"
        self.root = Builder.load_file("app.kv")

        return self.root

# with serial.Serial(port=PORT, baudrate=115200, timeout=0.5) as SerialPort:

#     while True:
#         for line in SerialPort.readlines():
#             print(line.strip().decode())
#         command = input('')
#         SerialPort.write(command.encode())

if __name__ == '__main__':
    print("Hello!!!!!")
    CNCApp().run()