import serial


PORT = "COM5"



with serial.Serial(port=PORT, baudrate=115200, timeout=0.5) as SerialPort:

    while True:
        for line in SerialPort.readlines():
            print(line.strip().decode())
        command = input('')
        SerialPort.write(command.encode())
