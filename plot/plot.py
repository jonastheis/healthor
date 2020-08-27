import os
import time
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.legend_handler import HandlerTuple


import utils


MESSAGES_GENERATED = 'generated'
MESSAGES_PROCESSED = 'processed'


class Plot:
    def __init__(self, config_name, network_name, simulation_time):
        self.plot_counts = {}
        self.time_string = time.strftime('%Y-%m-%dT%H.%M.%S')

        self.config_name = config_name
        self.network_name = network_name
        self.simulation_time = simulation_time
        self.path = os.path.join('out', '%s_%s' % (self.time_string, self.config_name))

        utils.run_simulation(config_name)
        utils.export_to_csv(config_name)

        if not os.path.exists(self.path):
            os.mkdir(self.path)

        # utils.save_simulation_state(self.path)

        self.csv = utils.parse_omnetpp_csv(config_name)
        self.run = self.csv.run.str.startswith(config_name)
        self.modules = self.csv.module.str.startswith(network_name, na=False)

    def __del__(self):
        print('Plotted to %s\n' % self.path)

    def save_to_file(self, group, name):
        if group in self.plot_counts:
            self.plot_counts[group] += 1
        else:
            self.plot_counts[group] = 0

        plt.savefig('%s/%s_%d_%s' % (self.path, group, self.plot_counts[group], name))
        plt.clf()

    def plot_throughput(self):
        processed = pd.DataFrame(columns=['msgID', 'time'])

        # read and combine all generated messages
        for f in utils.glob_csv_files(self.config_name, MESSAGES_PROCESSED):
            node = 'node[%s]' % f.split('_')[1][0]
            pro_node = pd.read_csv(f)
            pro_node['time'] = pd.to_numeric(pro_node["time"], downcast='float')
            pro_node['time'] = pro_node['time'] / 1000

            # total
            processed = processed.append(pro_node)

            count, division = np.histogram(pro_node['time'], bins=range(self.simulation_time))
            plt.plot(division, np.concatenate(([0], count)), label=node)

        count, division = np.histogram(processed['time'], bins=range(self.simulation_time))
        plt.plot(division, np.concatenate(([0], count)), label='total', linestyle='--')

        plt.title('Throughput')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend()
        self.save_to_file('throughput', 'all')

    def plot_generation_rate(self):
        generated = pd.DataFrame(columns=['msgID', 'time'])

        # read and combine all generated messages
        for f in utils.glob_csv_files(self.config_name, MESSAGES_GENERATED):
            node = 'node[%s]' % f.split('_')[1][0]
            gen_node = pd.read_csv(f)
            gen_node['time'] = pd.to_numeric(gen_node["time"], downcast='float')
            gen_node['time'] = gen_node['time'] / 1000

            # total
            generated = generated.append(gen_node)

            count, division = np.histogram(gen_node['time'], bins=range(self.simulation_time))
            plt.plot(division, np.concatenate(([0], count)), label=node)

        count, division = np.histogram(generated['time'], bins=range(self.simulation_time))
        plt.plot(division, np.concatenate(([0], count)), label='total', linestyle='--')

        plt.title('Generated messages')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend()
        self.save_to_file('generated_messages', 'all')

    def plot_available_processing(self):
        stats = self.csv.name == 'AvailableProcessingRate'

        available_processing = self.csv[self.run & self.modules & stats]
        total = np.zeros(available_processing.iloc[0].vectime.shape)
        for row in available_processing.itertuples():
            total += row.vecvalue
            node = row.module.split('.')[1]
            plt.plot(row.vectime, row.vecvalue, label=node)

        plt.plot(available_processing.iloc[0].vectime, total, label="total", linestyle='--')

        plt.title('Available processing rate')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend()
        self.save_to_file('available_processing', 'all')

    def plot_latency(self):
        generated = pd.DataFrame(columns=['msgID', 'time'])

        # read and combine all generated messages
        for f in utils.glob_csv_files(self.config_name, MESSAGES_GENERATED):
            c = pd.read_csv(f)
            generated = generated.append(c)

        generated["time"] = pd.to_numeric(generated["time"])
        totals = None

        # make sure we don't lose lines
        generated_count = generated.msgID.shape
        # get bin size
        max_bins = 0

        # get max diff and set as bin size
        for f in utils.glob_csv_files(self.config_name, MESSAGES_PROCESSED):
            node = 'node[%s]' % f.split('_')[1][0]
            c = pd.read_csv(f)
            merged = generated.merge(c, on='msgID')

            if merged.msgID.shape != generated_count:
                raise Exception(
                    'Messages in %s=%s do not match generated=%s' % (node, merged.msgID.shape, generated_count))

            # calculate diff
            merged['difference'] = merged.time_y.subtract(merged.time_x)

            if merged['difference'].max() > max_bins:
                max_bins = merged['difference'].max()

        # plot latency of every node
        for f in utils.glob_csv_files(self.config_name, MESSAGES_PROCESSED):
            node = 'node[%s]' % f.split('_')[1][0]
            c = pd.read_csv(f)
            merged = generated.merge(c, on='msgID')

            if merged.msgID.shape != generated_count:
                raise Exception(
                    'Messages in %s=%s do not match generated=%s' % (node, merged.msgID.shape, generated_count))

            # calculate diff
            merged['difference'] = merged.time_y.subtract(merged.time_x)
            # merged['difference'] = pd.to_numeric(merged['difference'], downcast='float')
            # merged['difference'] = merged['difference'] / 1000

            # compute cdf for more detailed inspection
            self.calculate_cdf(merged, node)

            if totals is None:
                totals = merged.copy()
            else:
                totals = totals.append(merged)

            plt.hist(merged.difference, bins=max_bins, linewidth=1, density=True, cumulative=True, label=node, histtype='step')

        # compute cdf for more detailed inspection
        self.calculate_cdf(totals, 'totals')

        plt.hist(totals['difference'], bins=max_bins, density=True, cumulative=True, label='total', histtype='step', linestyle='--')

        plt.title('Latencies')
        plt.ylabel('cdf')
        plt.xlabel('time (ms)')
        plt.legend()
        self.save_to_file('latencies', 'all')

    def calculate_cdf(self, df, node):
        stats_df = df.groupby(['difference'])['difference'].agg('count').pipe(pd.DataFrame).rename(
            columns={'difference': 'frequency'})
        stats_df['pdf'] = stats_df['frequency'] / sum(stats_df['frequency'])
        stats_df['cdf'] = stats_df['pdf'].cumsum()
        stats_df = stats_df.reset_index()
        utils.save_to_csv(stats_df, self.path, 'latency_' + node)


class PlotBasic(Plot):
    def __init__(self, config_name, network_name, simulation_time):
        super().__init__(config_name, network_name, simulation_time)

        self.plot_buffers()
        self.plot_throughput()
        self.plot_latency()
        self.plot_generation_rate()
        self.plot_available_processing()

    def plot_buffers(self):
        stats = self.csv.name == 'InboxLength'
        inbox_lengths = self.csv[self.run & self.modules & stats]

        for row in inbox_lengths.itertuples():
            node = row.module.split('.')[1]
            plt.plot(row.vectime, row.vecvalue, label=node)

        plt.title('Inbox length')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend()
        self.save_to_file('buffers', 'all')


class PlotV1(Plot):
    def __init__(self, config_name, network_name, simulation_time):
        super().__init__(config_name, network_name, simulation_time)

        # plot graphs
        self.plot_buffers()
        self.plot_throughput()
        self.plot_sending_rate()
        self.plot_latency()
        self.plot_generation_rate()
        self.plot_available_processing()

    def plot_buffers(self):
        group = 'buffers'
        totals = {}

        # plot inboxes
        stats = self.csv.name == 'InboxLength'
        inbox_lengths = self.csv[self.run & self.modules & stats]

        for row in inbox_lengths.itertuples():
            node = row.module.split('.')[1]
            totals[node] = row.vecvalue
            plt.plot(row.vectime, row.vecvalue, label='In ' + node)

        plt.title('Inboxes')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend()
        self.save_to_file(group, 'inboxes')

        # plot outboxes
        stats = self.csv.name.str.startswith('OutboxLength')
        outbox_lengths = self.csv[self.run & self.modules & stats]

        for row in outbox_lengths.itertuples():
            node = row.module.split('.')[1]
            outbox = row.name.split('.')[1]
            # totals[outbox] += row.vecvalue
            plt.plot(row.vectime, row.vecvalue, label=('Out %s -> %s' % (node, outbox)))

        plt.title('Outboxes')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend()
        self.save_to_file(group, 'outboxes')

        # TODO: figure out a way to interpolate or do histogram to be able to sum them together
        # # plot totals
        # for node, total in totals.items():
        #     plt.plot(range(len(total)), total, label='InOut ' + node)
        #
        # plt.title('Buffer totals')
        # plt.ylabel('messages')
        # plt.xlabel('time (s)')
        # plt.legend()
        # self.save_to_file(group, 'totals')

    def plot_sending_rate(self):
        stats = self.csv.name == 'Health'
        health = self.csv[self.run & self.modules & stats]

        fig, ax1 = plt.subplots()

        ax1.set_xlabel('time (s)')
        ax1.set_ylabel('health')
        ax1.tick_params(axis='y')

        for row in health.itertuples():
            ax1.plot(row.vectime, row.vecvalue, label='health ' + row.module.split('.')[1], linestyle='--')

        # instantiate a second axes that shares the same x-axis
        ax2 = ax1.twinx()

        ax2.set_ylabel('sending rate (M/s)')
        ax2.tick_params(axis='y')

        stats = self.csv.name.str.startswith('SendingRate')
        sending_rates = self.csv[self.run & self.modules & stats]

        for row in sending_rates.itertuples():
            node = row.module.split('.')[1]
            to = row.name.split('.')[1]
            ax2.plot(row.vectime, row.vecvalue, label=('Rate %s -> %s' % (node, to)))

        plt.title('Health vs. sending rate')
        ax1.legend()
        ax2.legend()
        fig.tight_layout()  # otherwise the right y-label is slightly clipped
        self.save_to_file('sending_rates', 'all')


class Serial20(Plot):
    def __init__(self, config_name, network_name, simulation_time):
        super().__init__(config_name, network_name, simulation_time)

    def plot_issue(self):
        # calculate non-disseminated messages at 10s and 60s
        times = [10000, 60000]
        time_frame = 5000

        generated = pd.DataFrame(columns=['msgID', 'time'])

        # read and combine all generated messages
        for f in utils.glob_csv_files(self.config_name, MESSAGES_GENERATED):
            c = pd.read_csv(f)
            generated = generated.append(c)

        generated["time"] = pd.to_numeric(generated["time"])
        totals = None

        # make sure we don't lose lines
        generated_count = generated.msgID.shape

        print(generated)

        for f in utils.glob_csv_files(self.config_name, MESSAGES_PROCESSED):
            node = 'node[%s]' % f.split('_')[1][0]
            c = pd.read_csv(f)
            merged = generated.merge(c, on='msgID')

            if merged.msgID.shape != generated_count:
                raise Exception(
                    'Messages in %s=%s do not match generated=%s' % (node, merged.msgID.shape, generated_count))

            # calculate diff
            merged['difference'] = merged.time_y.subtract(merged.time_x)

            for time in times:
                non_disseminated = merged[ (merged['time_x'] <= (time-time_frame)) & (merged['time_y'] > time) ]
                print('\n--------------------')
                print('%s: time:%d - time_frame:%d' % (node, time, time_frame))
                print(non_disseminated)
                print(non_disseminated['msgID'].count())

        return

        lines1 = []
        lines2 = []
        names = []

        fig, ax1 = plt.subplots()

        stats = self.csv.name == 'AvailableProcessingRate'

        available_processing = self.csv[self.run & self.modules & stats]
        for row in available_processing.itertuples():
            node = row.module.split('.')[1]
            names.append(node)
            markevery = 0.1
            if node == 'node[1]':
                marker = 'v'
                markevery = (0.05, 0.1)
            elif node == 'node[2]':
                marker = '.'
            else:
                marker = '^'
            l, = ax1.plot(row.vectime, row.vecvalue, label=node, linestyle='dashed', marker=marker, markevery=markevery)
            lines1.append(l)

        # instantiate a second axes that shares the same x-axis
        ax2 = ax1.twinx()

        stats = self.csv.name == 'InboxLength'
        inbox_lengths = self.csv[self.run & self.modules & stats]
        for row in inbox_lengths.itertuples():
            node = row.module.split('.')[1]
            markevery = 0.1
            if node == 'node[1]':
                marker = 'v'
                markevery = (0.05, 0.1)
            elif node == 'node[2]':
                marker = '.'
            else:
                marker = '^'
            l, = ax2.plot(row.vectime, row.vecvalue, label=node, marker=marker, markevery=markevery)
            lines2.append(l)

        ax1.axvline(60, alpha=0.8, linestyle='dotted', color='gray')

        ax1.set_xlabel('time (s)')
        ax1.set_ylabel('processing rate (messages)')
        ax1.tick_params(axis='y')
        ax1.set_ylim(bottom=0)

        ax2.set_ylabel('inbox buffer (messages)')
        ax2.tick_params(axis='y')
        ax2.set_ylim(bottom=0)

        # need to use ax1 for the legend bc otherwise 'tight_layout' clips it away
        # ax1.legend([(lines1[i], lines2[i]) for i in range(len(lines1))], names, handler_map={tuple: HandlerTuple(ndivide=None)},
        #            bbox_to_anchor=(0., 1.02, 1., .102), loc='lower left', ncol=3, mode="expand")
        ax1.legend(lines2, names,
                   bbox_to_anchor=(0., 1.02, 1., .102), loc='lower left', ncol=3, mode="expand")

        fig.set_dpi(300)
        fig.set_size_inches(6.4, 2.15)
        fig.tight_layout()  # otherwise the right y-label is slightly clipped

        # plt.title('Issue')
        # ax1.legend()
        self.save_to_file('procesing_vs_inbox_example', 'all')

    def plot_available_processing(self):
        node1 = None
        node2 = None
        others = None

        fig, (ax1, ax2) = plt.subplots(2)

        stats = self.csv.name == 'AvailableProcessingRate'

        available_processing = self.csv[self.run & self.modules & stats]
        for row in available_processing.itertuples():
            node = row.module.split('.')[1]
            if node == 'node[2]':
                node1, = ax1.plot(row.vectime, row.vecvalue, label=node, color='green', marker='.', markevery=0.1)
            elif node == 'node[4]':
                node2, =  ax1.plot(row.vectime, row.vecvalue, label=node, color='purple', marker='x', markevery=0.1)
            else:
                others, = ax1.plot(row.vectime, row.vecvalue, label='others', color='gray')

        ax1.set_ylabel('messages')
        ax1.set_title('Available processing rate')
        ax1.set_ylim(bottom=0)
        ax1.set_xticklabels([])  # remove x labels

        # need to use ax1 for the legend bc otherwise 'tight_layout' clips it away
        ax1.legend([node1, node2, others], ['node[2]', 'node[4]', 'others'], bbox_to_anchor=(0., 1.17, 1., .102), loc='lower left', ncol=3, mode="expand")

        # plot generated messages
        generated = pd.DataFrame(columns=['msgID', 'time'])

        # read and combine all generated messages
        for f in utils.glob_csv_files(self.config_name, MESSAGES_GENERATED):
            node = 'node[%s]' % f.split('_')[1][0]
            gen_node = pd.read_csv(f)
            gen_node['time'] = pd.to_numeric(gen_node["time"], downcast='float')
            gen_node['time'] = gen_node['time'] / 1000

            # total
            generated = generated.append(gen_node)

            count, division = np.histogram(gen_node['time'], bins=range(self.simulation_time))
            # print(node, count)
            # if count[0] == 0:
            #     ax2.plot(division, np.concatenate(([0], count)))
            # else:
            #     ax2.plot(division, np.concatenate(([0], count)), label=node)

        count, division = np.histogram(generated['time'], bins=range(self.simulation_time))
        ax2.plot(division, np.concatenate(([0], count)), label='total', linestyle=':')

        ax2.set_title('Total generated messages')
        ax2.set_ylabel('messages')
        ax2.set_xlabel('time (s)')

        fig.set_dpi(300)
        fig.set_size_inches(6.4, 4)
        fig.tight_layout()  # otherwise the right y-label is slightly clipped
        self.save_to_file('available_processing', 'all')

    def plot_inboxes(self):
        node1 = None
        node2 = None
        others = None

        fig, ax1 = plt.subplots()

        stats = self.csv.name == 'InboxLength'
        inbox_lengths = self.csv[self.run & self.modules & stats]

        for row in inbox_lengths.itertuples():
            node = row.module.split('.')[1]
            if node == 'node[2]':
                node1, = ax1.plot(row.vectime, row.vecvalue, label=node, color='green', marker='.', markevery=0.1)
            elif node == 'node[4]':
                node2, = ax1.plot(row.vectime, row.vecvalue, label=node, color='purple', marker='x', markevery=0.1)
            else:
                others, = ax1.plot(row.vectime, row.vecvalue, label='others', color='gray')


        # plot inboxes for V1
        config_name, network_name = 'Scenario1V1', 'Scenario1V1'

        self.csv = utils.parse_omnetpp_csv(config_name)
        self.run = self.csv.run.str.startswith(config_name)
        self.modules = self.csv.module.str.startswith(network_name, na=False)

        stats = self.csv.name == 'InboxLength'
        inbox_lengths = self.csv[self.run & self.modules & stats]

        for row in inbox_lengths.itertuples():
            node = row.module.split('.')[1]
            if node == 'node[2]':
                ax1.plot(row.vectime, row.vecvalue, label=node, color='green', marker='.', markevery=0.1, linestyle=(0, (5, 3)))
            elif node == 'node[4]':
                ax1.plot(row.vectime, row.vecvalue, label=node, color='purple', marker='x', markevery=0.1, linestyle=(0, (5, 3)))
            else:
                ax1.plot(row.vectime, row.vecvalue, label='others', color='gray')

        ax1.set_ylabel('inbox length (messages)')
        ax1.set_xlabel('time (s)')

        ax1.legend([node1, node2, others], ['node[2]', 'node[4]', 'others'], bbox_to_anchor=(0., 1.02, 1., .102), loc='lower left', ncol=3, mode="expand")

        fig.set_dpi(300)
        fig.set_size_inches(6.4, 3.3)
        fig.tight_layout()  # otherwise the right y-label is slightly clipped
        self.save_to_file('inboxes', 'all')

    def plot_throughput(self):
        self.config_name = 'Scenario1'

        node1 = None
        node2 = None
        others = None
        fig, (ax1, ax2) = plt.subplots(2)

        # read and combine all generated messages
        for f in utils.glob_csv_files(self.config_name, MESSAGES_PROCESSED):
            node = 'node[%s]' % f.split('_')[1][0]
            pro_node = pd.read_csv(f)
            pro_node['time'] = pd.to_numeric(pro_node["time"], downcast='float')
            pro_node['time'] = pro_node['time'] / 1000

            if node == 'node[2]':
                count, division = np.histogram(pro_node['time'], bins=range(self.simulation_time))
                node1, = ax1.plot(division, np.concatenate(([0], count)), label=node, color='green', marker='.', markevery=0.1)
            elif node == 'node[4]':
                count, division = np.histogram(pro_node['time'], bins=range(self.simulation_time))
                node2, = ax1.plot(division, np.concatenate(([0], count)), label=node, color='purple', marker='x', markevery=0.1)
            else:
                count, division = np.histogram(pro_node['time'], bins=range(self.simulation_time))
                others, = ax1.plot(division, np.concatenate(([0], count)), label=node, color='gray')

        ax1.set_ylabel('messages')
        ax1.set_title('Throughput without Healthor')
        ax1.set_xticklabels([])  # remove x labels

        ax1.legend([node1, node2, others], ['node[2]', 'node[4]', 'others'], bbox_to_anchor=(0., 1.17, 1., .102), loc='lower left', ncol=3, mode="expand")

        # plot throughput for V1
        self.config_name = 'Scenario1V1'
        # read and combine all generated messages
        for f in utils.glob_csv_files(self.config_name, MESSAGES_PROCESSED):
            node = 'node[%s]' % f.split('_')[1][0]
            pro_node = pd.read_csv(f)
            pro_node['time'] = pd.to_numeric(pro_node["time"], downcast='float')
            pro_node['time'] = pro_node['time'] / 1000

            if node == 'node[2]':
                count, division = np.histogram(pro_node['time'], bins=range(self.simulation_time))
                node1, = ax2.plot(division, np.concatenate(([0], count)), label=node, color='green', marker='.', markevery=0.1)
            elif node == 'node[4]':
                count, division = np.histogram(pro_node['time'], bins=range(self.simulation_time))
                node2, = ax2.plot(division, np.concatenate(([0], count)), label=node, color='purple', marker='x', markevery=0.1)
            else:
                count, division = np.histogram(pro_node['time'], bins=range(self.simulation_time))
                others, = ax2.plot(division, np.concatenate(([0], count)), label=node, color='gray')

        ax2.set_ylabel('messages')
        ax2.set_xlabel('time (s)')
        ax2.set_title('Throughput with Healthor')

        fig.set_dpi(300)
        fig.set_size_inches(6.4, 4)
        fig.tight_layout()  # otherwise the right y-label is slightly clipped
        self.save_to_file('throughput', 'all')