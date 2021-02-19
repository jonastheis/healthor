import os
import time
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.legend_handler import HandlerTuple
import datetime

import utils
import thesismain

MESSAGES_GENERATED = 'generated'
MESSAGES_PROCESSED = 'processed'


class Plot:
    def __init__(self, config_name, network_name, simulation_time, verbose_logs, run_simulation, plot_all):
        self.plot_counts = {}
        self.time_string = time.strftime('%Y-%m-%dT%H.%M.%S')

        self.config_name = config_name
        self.network_name = network_name
        self.simulation_time = simulation_time
        self.path = os.path.join('out', '%s_%s' % (self.time_string, self.config_name))
        self.verbose_logs = verbose_logs
        self.plot_all = plot_all

        if run_simulation:
            utils.run_simulation(config_name)
            utils.export_to_csv(config_name)

        if plot_all:
            if not os.path.exists(self.path):
                os.mkdir(self.path)

        # utils.save_simulation_state(self.path)

        self.csv = utils.parse_omnetpp_csv(config_name)
        self.run = self.csv.run.str.startswith(config_name)
        self.modules = self.csv.module.str.startswith(network_name, na=False)

        self.all_messages, self.all_nodes = self.prepare_all_messages()
        self.all_nodes_info = {}

        # get node information
        nodes_info = self.create_message_csv()
        for node in self.all_nodes:
            node_row = network_name+'.'+node
            self.all_nodes_info[node] = (nodes_info['processingType'][node_row], nodes_info['processingScale'][node_row])

    def __del__(self):
        if os.path.exists(self.path):
            print('Plotted to %s\n' % self.path)

    def save_to_file(self, group, name):
        if not os.path.exists(self.path):
            os.mkdir(self.path)

        if group in self.plot_counts:
            self.plot_counts[group] += 1
        else:
            self.plot_counts[group] = 0

        plt.savefig('%s/%s_%d_%s' % (self.path, group, self.plot_counts[group], name))
        plt.clf()

    def prepare_all_messages(self):
        generated = pd.DataFrame(columns=['msgID', 'time'])

        # read and combine all generated messages
        for f in utils.glob_csv_files(self.config_name, MESSAGES_GENERATED):
            c = pd.read_csv(f)
            generated = generated.append(c)

        generated["time"] = pd.to_numeric(generated["time"])
        # convert to seconds
        generated['time'] = generated['time'] / 1000

        generated.sort_values(by=['time', 'msgID'], inplace=True)

        # make sure we don't lose lines
        generated_count = generated.msgID.shape

        # get max diff and set as bin size
        for f in utils.glob_csv_files(self.config_name, MESSAGES_PROCESSED):
            node_id = int(f.split('_')[1][:-4])
            node = 'node[%s]' % node_id
            node_suffix = '_%s' % node
            node_time_column = 'time%s' % node_suffix
            c = pd.read_csv(f)
            generated = generated.merge(c, on='msgID', how='left', suffixes=[None, node_suffix])
            generated.rename(columns={node_time_column: node}, inplace=True)

            generated[node] = pd.to_numeric(generated[node])
            # convert to seconds
            generated[node] = generated[node] / 1000

            if generated.msgID.shape != generated_count:
                raise Exception('Messages in %s=%s do not match generated=%s' % (node, generated.msgID.shape, generated_count))

        # get all nodes
        cols = list(filter(lambda x: x.startswith('node'), list(generated)))

        # TODO: only needed for big files, 5000 FFA run
        # utils.save_to_csv(generated, 'path', 'msgs')

        return generated, cols

    def egalitarian_score(self, save_plot=True, label=None):
        TIME_LIMIT = 5
        msgs_df = self.all_messages.copy()

        # determine if node was in sync
        for node in self.all_nodes:
            msgs_df[node + '_sync'] = (msgs_df[node].subtract(msgs_df['time']) < TIME_LIMIT).astype(int)

        # calculate how many nodes were in sync
        nodes_sync = [x+'_sync' for x in self.all_nodes]
        msgs_df['totals'] = msgs_df[nodes_sync].sum(axis=1)

        # calculate normalized value of how many nodes were out of sync
        msgs_df['egalitarian_score'] = (len(self.all_nodes) - msgs_df['totals']) / len(self.all_nodes)

        if self.plot_all:
            utils.save_to_csv(msgs_df, self.path, 'egalitarian_score')

        df_plot = msgs_df[msgs_df['time'] < (self.simulation_time - TIME_LIMIT)]
        plt.plot(df_plot['time'], df_plot['egalitarian_score'], label=label, linewidth=0.9, markevery=3.5)

        # calculate mean of egalitarian score: leave away last interval bc results can be wrong due to messages not yet delivered before simulation ends
        mean = df_plot['egalitarian_score'].mean()
        print('Egalitarian score: %.2f' % mean)

        if save_plot:
            # set y-axis from 0 to max
            axes = plt.gca()
            # axes.set_ylim([0, 0.2])

            plt.title('Egalitarian Score=%.2f' % mean)
            plt.ylabel('nodes')
            plt.xlabel('time (s)')
            plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
            plt.subplots_adjust(right=0.81)
            self.save_to_file('egalitarian_score', 'all')

        return mean

    def plot_throughput(self):
        msgs_df = self.all_messages.copy()

        def plot_throughput(color_groups, plot_average_only):
            # determine how many nodes were in sync and average
            total_count = None
            total_in_sync_nodes = None
            total_division = None

            # plot nodes
            for node in self.all_nodes:
                color, label = self.get_line_color(node, color_groups)
                count, division = np.histogram(msgs_df[node], bins=range(self.simulation_time))

                if not plot_average_only:
                    plt.plot(division, np.concatenate(([0], count)), label=label, color=color, linewidth=0.5)

                # sum up for throughput average
                if total_count is not None:
                    total_count += count
                    total_in_sync_nodes += (count > 0).astype(int)
                else:
                    total_count = count
                    total_in_sync_nodes = (count > 0).astype(int)
                    total_division = division


            # determine average
            total_count = total_count / total_in_sync_nodes

            # plot average
            plt.plot(total_division, np.concatenate(([0], total_count)), label='avg', linewidth=1)

            if plot_average_only:
                # print mean value of average
                print('Throughput avg mean: %.2f' % np.mean(total_count))

            axes = plt.gca()
            axes.set_ylim([0, 250])

            plt.title('Throughput')
            plt.ylabel('messages')
            plt.xlabel('time (s)')
            plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
            plt.subplots_adjust(right=0.81)
            self.save_to_file('throughput', 'all')

        plot_throughput(False, False)
        plot_throughput(True, False)
        plot_throughput(True, True)

    def plot_generation_rate(self):
        generated = pd.DataFrame(columns=['msgID', 'time'])

        # read and combine all generated messages
        for f in utils.glob_csv_files(self.config_name, MESSAGES_GENERATED):
            node = 'node[%s]' % int(f.split('_')[1][:-4])
            gen_node = pd.read_csv(f)
            gen_node['time'] = pd.to_numeric(gen_node["time"], downcast='float')
            gen_node['time'] = gen_node['time'] / 1000

            # total
            generated = generated.append(gen_node)

            count, division = np.histogram(gen_node['time'], bins=range(self.simulation_time))
            plt.plot(division, np.concatenate(([0], count)), label=node, linewidth=0.5)

        count, division = np.histogram(generated['time'], bins=range(self.simulation_time))
        plt.plot(division, np.concatenate(([0], count)), label='total', linestyle='--')

        plt.title('Generated messages')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
        plt.subplots_adjust(right=0.81)
        self.save_to_file('generated_messages', 'all')

    def plot_available_processing(self):
        stats = self.csv.name == 'AvailableProcessingRate'

        available_processing = self.csv[self.run & self.modules & stats]
        # ignore totals for now
        # total = np.zeros(available_processing.iloc[0].vectime.shape)
        for row in available_processing.itertuples():
            # make sure to only add from nodes that did not go out of sync
            # if row.vecvalue.shape == available_processing.iloc[0].vectime.shape:
            #     total += row.vecvalue
            node = row.module.split('.')[1]
            plt.plot(row.vectime, row.vecvalue, label=node, linewidth=0.5)

        # plt.plot(available_processing.iloc[0].vectime, total, label="total", linestyle='--')

        plt.title('Available processing rate')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
        plt.subplots_adjust(right=0.81)
        self.save_to_file('available_processing', 'all')

    def plot_inbox_solidification_buffer(self):
        stats = self.csv.name == 'TotalInbox'
        inbox_lengths = self.csv[self.run & self.modules & stats]

        for row in inbox_lengths.itertuples():
            node = row.module.split('.')[1]
            plt.plot(row.vectime, row.vecvalue, label=node, linewidth=0.5)

        plt.title('Inbox+Solidification buffer length')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend()
        self.save_to_file('inbox_solidification', 'all')

    def thesismain_micro_inbox_solidification_buffer(self, annotation=None):
        thesismain.init_plot((16, 4))

        stats = self.csv.name == 'TotalInbox'
        inbox_lengths = self.csv[self.run & self.modules & stats]

        for row in inbox_lengths.itertuples():
            node = row.module.split('.')[1]
            plt.plot(row.vectime, row.vecvalue, label=node, linewidth=0.9, markevery=0.5)
            # find a specific point in a node's graph
            # if node == 'node[4]':
            #     print(np.where(row.vecvalue == 250)[0][-1])
            #     print(row.vectime[np.where(row.vecvalue == 250)[0][-1]])

        if annotation == 'Scenario2':
            plt.annotate('out-of-sync', xy=(54, 250), xycoords='data',
                         xytext=(75, 190), textcoords='data',
                         arrowprops=dict(arrowstyle="->"),
                         horizontalalignment='right', verticalalignment='top',
                         )
            plt.annotate('out-of-sync', xy=(120, 250), xycoords='data',
                         xytext=(135, 190), textcoords='data',
                         arrowprops=dict(arrowstyle="->"),
                         horizontalalignment='right', verticalalignment='top',
                         )

        plt.ylim(bottom=-5, top=260)

        plt.ylabel('messages')
        plt.xlabel('time (s)')

        plt.legend(bbox_to_anchor=(-0.17, 1.1, 1.2, .102), loc='lower left', ncol=10, mode="expand")

        thesismain.save_plot('plots/%s_%s_%s' % ('micro', 'inbox', self.config_name))

    def thesismain_micro_latency(self):
        msgs_df = self.all_messages.copy()
        difference_suffix = '_difference'

        # calculate latency for every message on every node
        for node in self.all_nodes:
            msgs_df[node + difference_suffix] = msgs_df[node].subtract(msgs_df['time'])

        cdfs = {}
        nodes_in_sync = []
        msgs_count = msgs_df.shape[0]
        for node in self.all_nodes:
            cdf_df = self.calculate_cdf(msgs_df, node)
            cdfs[node] = cdf_df
            msgs_node_missing = msgs_df[node].isnull().sum()
            if msgs_node_missing < msgs_count * 0.05:
                nodes_in_sync.append(node)

        thesismain.init_plot()

        # plot
        for node in self.all_nodes:
            cdf_df = cdfs[node]
            plt.plot(cdf_df[node + difference_suffix], cdf_df['cdf'], label=node, linewidth=0.9, markevery=0.2)

        plt.ylim(bottom=-0.02, top=1.02)
        plt.xlim(left=-0.2, right=6.2)

        plt.ylabel('cdf')
        plt.xlabel('time (s)')

        thesismain.save_plot('plots/%s_%s_%s' % ('micro', 'latency', self.config_name))

    def thesismain_micro_available_processing(self, annotation=None):
        thesismain.init_plot()

        stats = self.csv.name == 'AvailableProcessingRate'

        available_processing = self.csv[self.run & self.modules & stats]
        for row in available_processing.itertuples():
            node = row.module.split('.')[1]
            # find max's index and max value
            # print(np.where(row.vecvalue == row.vecvalue.max())[0][0], row.vecvalue.max())
            plt.plot(row.vectime, row.vecvalue, label=node, linewidth=0.9, markevery=0.1)

        # -> 89 845.0
        if annotation == 'Scenario2':
            plt.ylim(bottom=-5, top=310)
            plt.annotate('845 MPS', xy=(89, 302), xycoords='data',
                xytext=(129, 299), textcoords='data',
                arrowprops=dict(arrowstyle="->"),
                horizontalalignment='right', verticalalignment='top',
            )
        else:
            plt.ylim(bottom=-5)

        plt.ylabel('throughput (MPS)')
        plt.xlabel('time (s)')

        thesismain.save_plot('plots/%s_%s_%s' % ('micro', 'available-processing', self.config_name))

    def thesismain_micro_throughput(self, annotation=None):
        thesismain.init_plot()

        msgs_df = self.all_messages.copy()

        # determine how many nodes were in sync and average
        total_count = None
        total_in_sync_nodes = None
        total_division = None

        # plot nodes
        for node in self.all_nodes:
            label = node
            count, division = np.histogram(msgs_df[node], bins=range(self.simulation_time))

            plt.plot(division, np.concatenate(([0], count)), linewidth=0.9, markevery=0.1)

            # find a specific point in a node's graph
            # if node == 'node[6]':
            #     print(np.where(count == 0))

            # sum up for throughput average
            if total_count is not None:
                total_count += count
                total_in_sync_nodes += (count > 0).astype(int)
            else:
                total_count = count
                total_in_sync_nodes = (count > 0).astype(int)
                total_division = division


        # determine average
        total_count = total_count / total_in_sync_nodes

        # plot average
        plt.plot(total_division, np.concatenate(([0], total_count)), label='mean', linewidth=1.3, markevery=0.1)

        # print mean value of average
        print('Throughput avg mean: %.2f' % np.mean(total_count))

        if annotation == 'Scenario1':
            plt.annotate('out-of-sync', xy=(43, 0), xycoords='data',
                xytext=(85, 25), textcoords='data',
                arrowprops=dict(arrowstyle="->"),
                horizontalalignment='right', verticalalignment='top',
            )
        elif annotation == 'Scenario2':
            plt.annotate('out-of-sync', xy=(54, 0), xycoords='data',
                         xytext=(96, 25), textcoords='data',
                         arrowprops=dict(arrowstyle="->"),
                         horizontalalignment='right', verticalalignment='top',
                         )
            plt.annotate('out-of-sync', xy=(120, 0), xycoords='data',
                         xytext=(166, 25), textcoords='data',
                         arrowprops=dict(arrowstyle="->"),
                         horizontalalignment='right', verticalalignment='top',
                         )


        plt.ylim(bottom=-5, top=160)

        plt.ylabel('throughput (MPS)')
        plt.xlabel('time (s)')

        # plt.legend(bbox_to_anchor=(-0.12, 1.1, 1.14, .102), loc='lower left', ncol=5, mode="expand")
        plt.legend()
        thesismain.save_plot('plots/%s_%s_%s' % ('micro', 'throughput', self.config_name))


    def plot_solidification_buffer(self):
        stats = self.csv.name == 'SolidificationBuffer'

        buffer = self.csv[self.run & self.modules & stats]
        for row in buffer.itertuples():
            node = row.module.split('.')[1]
            plt.plot(row.vectime, row.vecvalue, label=node, linewidth=0.5)

        stats = self.csv.name == 'OutstandingMessageRequests'
        requests = self.csv[self.run & self.modules & stats]
        for row in requests.itertuples():
            node = row.module.split('.')[1]
            plt.plot(row.vectime, row.vecvalue, label=node, linestyle="--", linewidth=0.5)

        plt.title('Solidification buffer / Outstanding requests')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
        plt.subplots_adjust(right=0.81)
        self.save_to_file('solidification_buffer', 'all')

    def create_message_csv(self):
        # create result dataset
        name = 'generatedMessages'
        result = self.csv[self.run & self.modules & (self.csv.name == name)][['module']].copy().set_index('module')

        # add all scalar values
        processing_type_id = self.csv[self.run & self.modules & (self.csv.name == 'processingRateFile')].set_index('module')['value']
        processing_type_id.replace(to_replace=0.0, value='constant', inplace=True)
        processing_type_id.replace(to_replace=1.0, value='aws', inplace=True)
        processing_type_id.replace(to_replace=2.0, value='azure', inplace=True)
        processing_type_id.replace(to_replace=3.0, value='dimension_data', inplace=True)
        result['processingType'] = processing_type_id

        self.add_scalar_value(result, 'processingScale')
        self.add_scalar_value(result, 'generationRate')
        self.add_scalar_value(result, 'generatedMessages')
        self.add_scalar_value(result, 'droppedMessages')
        self.add_scalar_value(result, 'processedMessages')
        self.add_scalar_value(result, 'sentMessages')
        self.add_scalar_value(result, 'sentMessageRequests')
        self.add_scalar_value(result, 'sentMessageRequestResponses')
        self.add_scalar_value(result, 'receivedMessages')
        self.add_scalar_value(result, 'receivedMessageRequests')
        self.add_scalar_value(result, 'receivedMessageRequestResponses')
        if self.plot_all:
            utils.save_to_csv(result, self.path, 'messages')
        return result

    def add_scalar_value(self, result, name):
        data = self.csv[self.run & self.modules & (self.csv.name == name)].set_index('module')
        result[name] = data['value']

    def plot_latency(self):
        msgs_df = self.all_messages.copy()
        difference_suffix = '_difference'

        # calculate latency for every message on every node
        for node in self.all_nodes:
            msgs_df[node+difference_suffix] = msgs_df[node].subtract(msgs_df['time'])

        cdfs = {}
        nodes_in_sync = []
        msgs_count = msgs_df.shape[0]
        for node in self.all_nodes:
            cdf_df = self.calculate_cdf(msgs_df, node)
            cdfs[node] = cdf_df
            msgs_node_missing = msgs_df[node].isnull().sum()
            if msgs_node_missing < msgs_count * 0.05:
                nodes_in_sync.append(node)

        def plot_latency_percentile(percentiles):
            nodes_difference_columns = [x + difference_suffix for x in nodes_in_sync]

            for percentile in percentiles:
                percentile_name = 'percentile_' + str(percentile)
                percentile_name_difference = percentile_name + '_difference'
                percentile_df = pd.DataFrame(columns=[percentile_name_difference])
                percentile_df[percentile_name_difference] = msgs_df[nodes_difference_columns].quantile(percentile, axis=1)

                cdf_df = self.calculate_cdf(percentile_df, percentile_name)
                plt.plot(cdf_df[percentile_name_difference], cdf_df['cdf'], label=str(percentile), linewidth=0.7)

                # print mean value of CDF
                np_arr = cdf_df['cdf'].to_numpy()
                m_index = np.where(np_arr >= 0.5)[0][0]
                print('Percentile CDF %.2f mean: %.2f' % (percentile, cdf_df[percentile_name_difference][m_index]))

            plt.title('Latencies percentile of in-sync nodes')
            plt.ylabel('latency (s)')
            plt.xlabel('time')
            plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
            plt.subplots_adjust(right=0.81)
            self.save_to_file('latencies', 'percentiles')

        def plot_latency_cdf(color_groups, sync_nodes_only=False):
            for node in self.all_nodes:
                if not sync_nodes_only or (sync_nodes_only and node in nodes_in_sync):
                    color, label = self.get_line_color(node, color_groups)
                    cdf_df = cdfs[node]
                    plt.plot(cdf_df[node+difference_suffix], cdf_df['cdf'], label=label, color=color, linewidth=0.7)

            file_name = 'latencies'
            if sync_nodes_only:
                file_name = file_name + '_sync_only'

            plt.title(file_name)
            plt.ylabel('cdf')
            plt.xlabel('time')
            plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
            plt.subplots_adjust(right=0.81)

            self.save_to_file(file_name, 'all')

        def plot_latency_over_time(color_groups):
            # plot latency over time
            for node in self.all_nodes:
                color, label = self.get_line_color(node, color_groups)
                plt.plot(msgs_df['time'], msgs_df[node+difference_suffix], label=label, color=color, linewidth=0.5)

            plt.title('Latencies over time')
            plt.ylabel('latency (s)')
            plt.xlabel('time')
            plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
            plt.subplots_adjust(right=0.81)
            self.save_to_file('latencies', 'over_time')

        plot_latency_cdf(False)
        plot_latency_cdf(True)
        plot_latency_cdf(False, sync_nodes_only=True)
        plot_latency_cdf(True, sync_nodes_only=True)
        plot_latency_percentile([0.5, 0.9, 0.95])
        plot_latency_over_time(False)
        plot_latency_over_time(True)

    def get_line_color(self, node, color_groups):
        if not color_groups:
            return None, node

        processing_scale = self.all_nodes_info[node][1]
        if 0 <= processing_scale < 1:
            return 'red', '0<=x<1'
        elif 1 <= processing_scale < 1.2:
            return 'orange', '1<=x<1.2'
        elif 1.2 <= processing_scale < 1.5:
            return 'blue', '1.2<=x<1.5'
        elif 1.5 <= processing_scale:
            return 'green', '1.5<=x'

    def calculate_cdf(self, df, node):
        node_difference = node+'_difference'
        stats_df = df.groupby([node_difference])[node_difference].agg('count').pipe(pd.DataFrame).rename(columns={node_difference: 'frequency'})
        stats_df['pdf'] = stats_df['frequency'] / sum(stats_df['frequency'])
        stats_df['cdf'] = stats_df['pdf'].cumsum()
        stats_df = stats_df.reset_index()

        if not os.path.exists(self.path):
            os.mkdir(self.path)

        utils.save_to_csv(stats_df, self.path, 'latency_' + node)
        return stats_df


class PlotBasic(Plot):
    def __init__(self, config_name, network_name, simulation_time, verbose_logs=True, run_simulation=True, plot_all=True):
        super().__init__(config_name, network_name, simulation_time, verbose_logs, run_simulation, plot_all)

        if plot_all:
            if verbose_logs:
                self.plot_buffers()
                self.plot_available_processing()
                self.plot_solidification_buffer()
                self.plot_inbox_solidification_buffer()

            self.plot_throughput()
            self.plot_latency()

            self.plot_generation_rate()
            self.egalitarian_score()

    def plot_buffers(self):
        stats = self.csv.name == 'InboxLength'
        inbox_lengths = self.csv[self.run & self.modules & stats]

        for row in inbox_lengths.itertuples():
            node = row.module.split('.')[1]
            plt.plot(row.vectime, row.vecvalue, label=node, linewidth=0.5)

        plt.title('Inbox length')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend()
        self.save_to_file('buffers', 'all')


class PlotV1(Plot):
    def __init__(self, config_name, network_name, simulation_time, verbose_logs=True, run_simulation=True, plot_all=True):
        super().__init__(config_name, network_name, simulation_time, verbose_logs, run_simulation, plot_all)

        if plot_all:
            if verbose_logs:
                self.plot_buffers()
                self.plot_available_processing()
                self.plot_solidification_buffer()
                self.plot_inbox_solidification_buffer()
                self.plot_sending_rate()

            self.plot_throughput()
            self.plot_latency()

            self.plot_generation_rate()
            self.egalitarian_score()

    def create_message_csv(self):
        # create result dataset
        name = 'generatedMessages'
        result = self.csv[self.run & self.modules & (self.csv.name == name)][['module']].copy().set_index('module')

        # add all scalar values
        processing_type_id = \
            self.csv[self.run & self.modules & (self.csv.name == 'processingRateFile')].set_index('module')['value']
        processing_type_id.replace(to_replace=0.0, value='constant', inplace=True)
        processing_type_id.replace(to_replace=1.0, value='aws', inplace=True)
        processing_type_id.replace(to_replace=2.0, value='azure', inplace=True)
        processing_type_id.replace(to_replace=3.0, value='dimension_data', inplace=True)
        result['processingType'] = processing_type_id

        self.add_scalar_value(result, 'processingScale')
        self.add_scalar_value(result, 'generationRate')
        self.add_scalar_value(result, 'generatedMessages')
        self.add_scalar_value(result, 'droppedMessages')
        self.add_scalar_value(result, 'processedMessages')
        self.add_scalar_value(result, 'sentMessages')
        self.add_scalar_value(result, 'sentMessageRequests')
        self.add_scalar_value(result, 'sentMessageRequestResponses')
        self.add_scalar_value(result, 'receivedMessages')
        self.add_scalar_value(result, 'receivedMessageRequests')
        self.add_scalar_value(result, 'receivedMessageRequestResponses')
        self.add_scalar_value(result, 'sentHealthMessages')
        self.add_scalar_value(result, 'receivedHealthMessages')

        # TODO: there are some issues with the matrix, therefore ignore for now
        # nodes = list(result.index.values)
        # for n in nodes:
        #     self.add_scalar_value(result, 'droppedMessages-%s' % n)
        if self.plot_all:
            utils.save_to_csv(result, self.path, 'messages')
        return result

    def plot_buffers(self, to_node=None):
        group = 'buffers'
        totals = {}

        # plot inboxes
        stats = self.csv.name == 'InboxLength'
        inbox_lengths = self.csv[self.run & self.modules & stats]

        for row in inbox_lengths.itertuples():
            node = row.module.split('.')[1]
            totals[node] = row.vecvalue
            plt.plot(row.vectime, row.vecvalue, label='In ' + node, linewidth=0.5)

        plt.title('Inboxes')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
        plt.subplots_adjust(right=0.79)
        self.save_to_file(group, 'inboxes')

        # plot outboxes
        stats = self.csv.name.str.startswith('OutboxLength')
        outbox_lengths = self.csv[self.run & self.modules & stats]

        for row in outbox_lengths.itertuples():
            node = row.module.split('.')[1]
            outbox = row.name.split('.')[1]
            if to_node is None or to_node == outbox:
                plt.plot(row.vectime, row.vecvalue, label=('O %s>%s' % (node, outbox)), linewidth=0.5)

        plt.title('Outboxes')
        plt.ylabel('messages')
        plt.xlabel('time (s)')
        plt.legend(bbox_to_anchor=(1, 1), loc="upper left")
        plt.subplots_adjust(right=0.70)
        if to_node is None:
            self.save_to_file(group, 'outboxes')
        else:
            self.save_to_file(group, 'outboxes' + to_node)

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
            ax1.plot(row.vectime, row.vecvalue, label='health ' + row.module.split('.')[1], linestyle='--', linewidth=0.5)

        # instantiate a second axes that shares the same x-axis
        ax2 = ax1.twinx()

        ax2.set_ylabel('sending rate (M/s)')
        ax2.tick_params(axis='y')

        stats = self.csv.name.str.startswith('SendingRate')
        sending_rates = self.csv[self.run & self.modules & stats]

        for row in sending_rates.itertuples():
            node = row.module.split('.')[1]
            to = row.name.split('.')[1]
            ax2.plot(row.vectime, row.vecvalue, label=('R %s>%s' % (node, to)), linewidth=0.5)

        plt.title('Health vs. sending rate')
        ax1.legend()
        ax2.legend(bbox_to_anchor=(1, 1), loc="upper left")
        fig.tight_layout()  # otherwise the right y-label is slightly clipped
        plt.subplots_adjust(right=0.70)
        self.save_to_file('sending_rates', 'all')

    def plot_allowed_receiving_rate(self, print_node=None, malicious_node=None, annotation=None):
        thesismain.init_plot()

        # plot inboxes
        stats = self.csv.name == 'AllowedReceivingRate'
        allowed_receiving_rate = self.csv[self.run & self.modules & stats]

        print_node_values = None
        for row in allowed_receiving_rate.itertuples():
            node = row.module.split('.')[1]
            if print_node is None or print_node == node:
                print_node_values = row.vecvalue
                plt.plot(row.vectime, row.vecvalue, label='allowed', linewidth=1.7, markevery=0.2)

        # plot actual receiving rate
        stats = self.csv.name.str.startswith('ReceivingRate')
        actual_receiving_rates = self.csv[self.run & self.modules & stats]

        for row in actual_receiving_rates.itertuples():
            receiver = row.module.split('.')[1]
            sender = row.name.split('.')[1]

            if print_node is None or print_node == receiver:
                # find a specific point in a node's graph
                # if sender == 'node[7]':
                #     print(len(row.vectime))
                #     print(row.vecvalue[-1])

                # if malicious_node == sender:
                #     for count, val in enumerate(row.vecvalue):
                #         if len(print_node_values) > count:
                #             if val > print_node_values[count]:
                #                 print('%d: %0.2f %0.2f <--' % (count, print_node_values[count], val))
                #             else:
                #                 print('%d: %0.2f %0.2f' % (count, print_node_values[count], val))
                plt.plot(row.vectime, row.vecvalue, label=('%s' % (sender)), linewidth=0.9)

        # plt.ylim(bottom=-5, top=260)

        if annotation == 'ExceedSendingRateAttack':
            plt.annotate('dropped', xy=(20, 110.66), xycoords='data',
                xytext=(27, 60), textcoords='data',
                arrowprops=dict(arrowstyle="->"),
                horizontalalignment='right', verticalalignment='top',
            )
        elif annotation == 'SubceedSendingRateAttack':
            plt.annotate('dropped', xy=(48, 10), xycoords='data',
                         xytext=(60, 40), textcoords='data',
                         arrowprops=dict(arrowstyle="->"),
                         horizontalalignment='right', verticalalignment='top',
                         )

        plt.ylabel('rate (MPS)')
        plt.xlabel('time (s)')

        # plt.legend(bbox_to_anchor=(-0.12, 1.1, 1.14, .102), loc='lower left', ncol=5, mode="expand")

        thesismain.save_plot('plots/%s_%s_%s' % ('attack', 'receiving_rate', self.config_name))

    def plot_outboxes(self, print_node=None, annotation=None):
        thesismain.init_plot()

        # plot outboxes
        stats = self.csv.name.str.startswith('OutboxLength')
        outbox_lengths = self.csv[self.run & self.modules & stats]

        for row in outbox_lengths.itertuples():
            node = row.module.split('.')[1]
            outbox = row.name.split('.')[1]
            if print_node is None or node == print_node:
                plt.plot(row.vectime, row.vecvalue, label=outbox, linewidth=0.9, markevery=0.1)
                if outbox == 'node[1]':
                    print(len(row.vectime))
                    print(row.vecvalue[-1])

        plt.ylim(top=260)
        axes = plt.axes()
        axes.set_yticks([0, 100, 200, 250])

        if annotation == 'LowHealthAttack':
            plt.annotate('dropped', xy=(17, 250), xycoords='data',
                xytext=(25, 200), textcoords='data',
                arrowprops=dict(arrowstyle="->"),
                horizontalalignment='right', verticalalignment='top',
            )

        plt.ylabel('messages')
        plt.xlabel('time (s)')

        plt.legend(bbox_to_anchor=(-0.12, 1.1, 1.14, .102), loc='lower left', ncol=5, mode="expand")

        thesismain.save_plot('plots/%s_%s_%s' % ('attack', 'outboxes', self.config_name))


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