"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
Copyright (c) 2020-2021 Ali Bohloolizamani

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
import serial
import time
import matplotlib.pyplot as plt

serial_speed = 9600
serial_port = '/dev/tty.BT18-SPPslave' # BLE

if __name__ == '__main__':
    bT = serial.Serial(serial_port, serial_speed, timeout=1)
    fig, ax = plt.subplots(nrows=2, ncols=1);
    fig.suptitle('CoolTag 2021', fontsize=16);
    t = range(0, 256);
    plt.ion();
    bT.write("CoolTag".encode("utf-8"));
    try:
        while (1):
            bTData = bT.readline();
            if (len(bTData) == 0):
                continue;
            if (len(bTData) == 1024):
                results = list(map(int, bTData));
                humidity = [];
                temperature = [];
                temp_t = 0.0;
                temp_h = 0.0;
                for i in range(0, len(results)):
                    if (i % 4 == 0):
                        temp_h = results[i] - ord(' ');
                    elif (i % 4 == 1):
                        humidity.append(((results[i] - ord(' ')) / 100) + temp_h);
                    elif (i % 4 == 2):
                        temp_t = results[i] - ord(' ');
                    else:
                        temperature.append(((results[i] - ord(' ')) / 100) + temp_t);
                ax[0].set_ylabel("Humidity %RH");
                ax[0].set_xlabel("Time 1Sec");
                ax[0].autoscale_view(True, True, True);
                ax[0].plot(t, humidity);
                ax[1].set_ylabel("Temperature " + u"\u2103");
                ax[1].set_xlabel("Time 1Sec");
                ax[1].autoscale_view(True, True, True);
                ax[1].plot(t, temperature);
                plt.draw();
                plt.pause(5);
                ax[0].clear();
                ax[1].clear();
                bT.write("CoolTag".encode("utf-8"));
            else:
                print(bTData);
    except Exception as ex:
        print(ex);
        print("\ndone!");
