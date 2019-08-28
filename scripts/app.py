'''
This python script listens on UDP port 3333
for messages from the ESP32 board and prints them
'''
import socketserver
import threading
import queue
import struct
import signal
from statistics import median
from math import sqrt
import matplotlib.pyplot as plt
import matplotlib.style as mplstyle
from scipy.signal import savgol_filter, butter, sosfilt, cheby2
from scipy.io import savemat
import numpy as np

# fast and light-weight plotting style
# https://matplotlib.org/tutorials/introductory/usage.html#sphx-glr-tutorials-introductory-usage-py
mplstyle.use(['dark_background', 'fast'])

shutdownEvent = threading.Event()
q = queue.Queue()
x_data = []
y_data = []
z_data = []

INT16_MAX = 2**15-1
UINT16_MAX = 2**16
MAX_RECORD = 4
HAMPEL_WINDOW = 5
SG_FILTER_WINDOW = 11
SG_COFF = [-42,-21,-2,15,30,43,54,63,70,75,78,79,78,75,70,63,54,43,30,15,-2,-21,-42]
SG_H = 805
#SG_COFF = [-253,-138,-33,62,147,222,287,343,387,422,447,462,467,462,444,422,387,343,287,222,147,62,-33,-138,-253]
#SG_H = 5175


class MyHandler(socketserver.BaseRequestHandler):
    '''
    handle the UDP request
    '''
    def handle(self):
        global count
        data = self.request.recv(1024).strip()
        data = struct.unpack('<ddd', data)
        if (len(data) == 3):
            x_data.append(data[0])
            y_data.append(data[1])
            z_data.append(data[2])
        if (len(x_data) % 50 == 0):
            print('Total: {:}'.format(len(x_data)))

def keyboardInterrupt(signal, frame):
    shutdownEvent.set()
    print("Keyboard Interrupt {:}".format(signal))

def reader():
    count = 0
    while True:
        try:
            # if empty queue, the exception will raise
            item = q.get(block=False, timeout=1000)
            ch1, ch2 = item
            if ch1 is not None:
                ch1_data.extend(ch1)
            if ch2 is not None:
                ch2_data.extend(ch2)
            q.task_done()
            # stop condition
            count += 1
            if count >= MAX_RECORD:
                shutdownEvent.set()
        except queue.Empty:
            if shutdownEvent.is_set():
                break
            
def hampel(data = [], window=HAMPEL_WINDOW, k=1.5):
    length_data = len(data)
    filtered = []
    for i in range(length_data):
            # Hampel filter: remove outliner
            if i-window >= 0:
                start_i = i-window
            else:
                start_i = 0
            seq = data[start_i:i+window]
            if len(seq) > 0:
                m = median(seq)
                sd = 0
                for x in seq:
                    sd += (x - m)**2
                sd = sqrt(sd / len(seq))
                if abs(data[i] - m) > k*sd:
                    filtered.append(m)
                else:
                    filtered.append(data[i])
    return filtered

def smooth(data=[]):
    filtered = []
    length_data = len(data)
    for i in range(length_data):
        # Savitzky-Golay filter
        if i-SG_FILTER_WINDOW >= 0:
            start_i = i-SG_FILTER_WINDOW
        else:
            start_i = 0
        seq = data[start_i:i+SG_FILTER_WINDOW]
        _sum = 0
        for ii in range(len(seq)):
            _sum += seq[ii] * SG_COFF[ii]
        filtered.append(_sum/SG_H)
    return filtered

def kalman(data=[]):
    Q = 1e-5 # process variance
    # allocate space for arrays
    xhat=[data[0]]
    P=[1.0]
    xhatminus=[0]
    Pminus=[0]
    K=[0]
    R = 0.0001 # estimate of measurement variance, change to see effect

    for k in range(1,len(data)):
        # time update
        xhatminus.append(xhat[k-1])
        Pminus.append(P[k-1]+Q)

        # measurement update
        K.append( Pminus[k]/( Pminus[k]+R ) )
        xhat.append( xhatminus[k]+K[k]*(data[k]-xhatminus[k]) )
        P.append( (1-K[k])*Pminus[k] )
    return xhat

if __name__ == "__main__":
    HOST, PORT = "0.0.0.0", 8080
    signal.signal(signal.SIGINT, keyboardInterrupt)
    signal.signal(signal.SIGBREAK, keyboardInterrupt)

    with socketserver.ThreadingTCPServer((HOST, PORT), MyHandler) as server:
        # use this in threading
        server_thread = threading.Thread(target=server.serve_forever)
        server_thread.daemon = True
        server_thread.start()
        print('Server Listening: {}'.format(server.server_address))
        # reader in threading
        # reader_thread = threading.Thread(target=reader)
        # reader_thread.start()
        # normal blocking server
        while True:
            if shutdownEvent.wait(1):
                break
        # q.join()
        server.shutdown()
        # reader_thread.join()
        # after stop show plot
        sos = butter(2, 0.5, 'lowpass', fs=10, output='sos')
        x_filt = savgol_filter(x_data, 11, 7)
        y_filt = savgol_filter(y_data, 11, 7)
        z_filt = savgol_filter(z_data, 11, 7)
        plt.plot(x_filt, '-.', label='X')
        plt.plot(y_filt, '-.', label='Y')
        plt.plot(z_filt, '--', label='Z')
        total_g = np.array([x_filt, y_filt], dtype='double')
        total_g = np.linalg.norm(total_g, axis=0)
        plt.plot(total_g, '-', label='Total')
        plt.legend()
        plt.show()

        filename = input('Enter filename to save: ')
        if len(filename) > 0:
            savemat(filename, {'x': x_data, 'y': y_data, 'z': z_data, 'x_filt': x_filt, 'y_filt': y_filt, 'z_filt': z_filt})