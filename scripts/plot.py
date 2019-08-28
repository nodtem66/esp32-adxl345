import sys
import matplotlib.pyplot as plt
import matplotlib.style as mplstyle
from scipy.signal import savgol_filter, butter, sosfilt, cheby2
from scipy.io import loadmat
import numpy as np

# fast and light-weight plotting style
# https://matplotlib.org/tutorials/introductory/usage.html#sphx-glr-tutorials-introductory-usage-py
mplstyle.use(['dark_background', 'fast'])

if __name__ == '__main__':
    if len(sys.argv) > 1:
        filename = sys.argv[1]
        print('Loading file: {:}'.format(filename))
        data = loadmat(filename)
        x_filt = data['x_filt'][0,:]
        y_filt = data['y_filt'][0,:]
        z_filt = data['z_filt'][0,:]
        plt.plot(x_filt, '-.', label='X')
        plt.plot(y_filt, '-.', label='Y')
        plt.plot(z_filt, '--', label='Z')
        #total_g = np.array([x_filt, y_filt], dtype='double')
        #total_g = np.linalg.norm(total_g, axis=0)
        #plt.plot(total_g, '-', label='Total')
        plt.legend()
        plt.show()