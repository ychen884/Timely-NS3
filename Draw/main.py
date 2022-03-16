# This is pytho script to draw graph
import matplotlib
import matplotlib.pyplot as plt


# queue, RTT, gradient
if __name__ == '__main__':
    val = input("Enter your txt file to draw: ")
    file1 = open(val, 'r')
    G = []
    Q = []
    R = []
    T_G = []
    T_Q = []
    T_R = []
    checked_1 = 1
    checked_2 = 1
    for eachl in file1:
        if eachl[0] == '~' or checked_1 == 1:
            checked_1 = 0
            continue
        if checked_2 == 1:
            checked_2 = 0
            continue
        data = eachl.split(',')
        if data[0] == 'G':
            G.append(float(data[1]))
            T_G.append(int(data[2]))
        if data[0] == 'Q':
            Q.append(float(data[1]))
            T_Q.append(int(data[2]))
        if data[0] == 'RTT':
            R.append(float(data[1]))
            T_R.append(int(data[2]))

    plt.plot(T_G, G, label='Normalized gradient vs Time')
    plt.title("Normalized gradient vs Time")
    plt.xlabel("Time in microsecond")
    plt.ylabel("RTT in microsecond")
    plt.show()

    plt.plot(T_R, R, label='RTT vs Time')
    plt.title("RTT vs Time")
    plt.xlabel("Time in microsecond")
    plt.ylabel("RTT in microsecond")
    plt.show()

    plt.plot(T_Q, Q, label='Number of packets in queue vs Time')
    plt.title("Number of packets in queue vs Time")
    plt.xlabel("Time in microsecond")
    plt.ylabel("Packets number")
    plt.show()

