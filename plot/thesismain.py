import argparse
import sys

import matplotlib.pyplot as plt
from cycler import cycler
import numpy as np
import pandas as pd

import plot


MACRO_AIDED = 'aided'
MACRO_UNAIDED = 'unaided'
MACRO_HEALTHOR = 'Healthor'
MACRO_HEALTHOR_UNAIDED = 'Healthor unaided'


def init_plot(figsize=None):
    if figsize is None:
        figsize = (8, 4)
    f = plt.figure(figsize=figsize, dpi=80)

    font = {
        'weight': 'normal',
        'size': 16,
    }
    plt.rc('font', **font)

    # prepare plots with markers so that they're readable in black and white
    monochrome = (
        cycler('marker', ['^', 'v', '<', '>', 'x', 'd', "+", 'p', "*", '1', '2', '3', '4']) +
        cycler('color', ['#1177b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd', '#8c564b', '#e377c2', '#717f7f', '#bcbd22', '#17becf', 'k', 'k', 'k']))
    plt.rc('axes', prop_cycle=monochrome)

    # enable grid
    plt.grid(axis='y', alpha=0.7)

    return f


def save_plot(name, view_only=False):
    plt.tight_layout()
    if view_only:
        plt.show()
    else:
        plt.savefig('out/t/%s.pdf' % name, bbox_inches='tight')
    plt.clf()


def plot_throughput_mean(plot_object, label, plot=True):
    # determine how many nodes were in sync and average
    total_count = None
    total_in_sync_nodes = None
    total_division = None

    msgs_df = plot_object.all_messages

    # plot nodes
    for node in plot_object.all_nodes:
        count, division = np.histogram(msgs_df[node], bins=range(plot_object.simulation_time))

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

    print('Throughput avg mean: %.2f' % np.mean(total_count))

    if plot:
        # plot average
        plt.plot(total_division, np.concatenate(([0], total_count)), label=label, linewidth=1.7, markevery=0.1)


def plot_latency_percentile(plot_object, label, percentiles=[0.95], plot=True):
        msgs_df = plot_object.all_messages.copy()
        difference_suffix = '_difference'

        # calculate latency for every message on every node
        for node in plot_object.all_nodes:
            msgs_df[node+difference_suffix] = msgs_df[node].subtract(msgs_df['time'])

        cdfs = {}
        nodes_in_sync = []
        msgs_count = msgs_df.shape[0]
        for node in plot_object.all_nodes:
            cdf_df = plot_object.calculate_cdf(msgs_df, node)
            cdfs[node] = cdf_df
            msgs_node_missing = msgs_df[node].isnull().sum()
            if msgs_node_missing < msgs_count * 0.05:
                nodes_in_sync.append(node)

        nodes_difference_columns = [x + difference_suffix for x in nodes_in_sync]

        for percentile in percentiles:
            percentile_name = 'percentile_' + str(percentile)
            percentile_name_difference = percentile_name + '_difference'
            percentile_df = pd.DataFrame(columns=[percentile_name_difference])
            percentile_df[percentile_name_difference] = msgs_df[nodes_difference_columns].quantile(percentile, axis=1)

            cdf_df = plot_object.calculate_cdf(percentile_df, percentile_name)
            if plot:
                plt.plot(cdf_df[percentile_name_difference], cdf_df['cdf'], label=label, linewidth=1.7, markevery=0.1)

            # print mean value of CDF
            np_arr = cdf_df['cdf'].to_numpy()
            m_index = np.where(np_arr >= 0.95)[0][0]
            print('Percentile CDF %.2f 0.95th: %.2f' % (percentile, cdf_df[percentile_name_difference][m_index]))


def print_latency_percentile_node(plot_object, node_name):
    msgs_df = plot_object.all_messages.copy()
    difference_suffix = '_difference'

    # calculate latency for every message on every node
    for node in plot_object.all_nodes:
        msgs_df[node + difference_suffix] = msgs_df[node].subtract(msgs_df['time'])

    cdfs = {}
    for node in plot_object.all_nodes:
        cdf_df = plot_object.calculate_cdf(msgs_df, node)
        cdfs[node] = cdf_df

    np_arr = cdfs[node_name]['cdf'].to_numpy()
    m_index = np.where(np_arr >= 0.95)[0][0]
    print('Percentile 0.95th %s: %.2f' % (node_name, cdfs[node_name][node_name + difference_suffix][m_index]))


def plot_egalitarian_gradient(plot_object, label, plot=True):
    return plot_object.egalitarian_score(plot, label)


def sec2_processing_variability():
    init_plot()

    df_aws = pd.read_csv('raw_data/aws.csv')
    # print(df_aws['rate'].idxmax(), df_aws['rate'].max())  # -> 219, 6.5
    df_azure = pd.read_csv('raw_data/azure.csv')
    df_dimension_data = pd.read_csv('raw_data/dimension_data.csv')

    plt.plot(df_aws['second'], df_aws['rate'], label="AWS", linewidth=0.9, markevery=0.5)
    plt.plot(df_azure['second'], df_azure['rate'], label="Azure", linewidth=0.9, markevery=0.5)
    plt.plot(df_dimension_data['second'], df_dimension_data['rate'], label="Dimension Data", linewidth=0.9, markevery=0.5)

    plt.ylim(bottom=-0.1, top=1.5)
    plt.annotate('6.5', xy=(219, 1.48),  xycoords='data',
            xytext=(320, 1.35), textcoords='data',
            arrowprops=dict(arrowstyle="->"),
            horizontalalignment='right', verticalalignment='top',
    )

    plt.ylabel('Processing variability')
    plt.xlabel('time (h)')

    plt.legend(loc='lower right')

    save_plot('plots/hetero_processing_variability')


def sec2_throughput(basic, ffa):
    init_plot()
    plot_throughput_mean(basic, MACRO_AIDED)
    plot_throughput_mean(ffa, MACRO_UNAIDED)

    plt.ylabel('throughput (MPS)')
    plt.xlabel('time (s)')

    plt.legend()

    save_plot('plots/hetero_throughput_basic_vs_ffa')


def sec2_egalitarian_gradient(basic, ffa):
    init_plot()

    mean = basic.egalitarian_score(False, 'aided')
    print('Egalitarian score basic: %.2f' % mean)
    mean = ffa.egalitarian_score(False, 'unaided')
    print('Egalitarian score FFA: %.2f' % mean)

    plt.ylabel('score')
    plt.xlabel('time (s)')

    plt.legend()

    save_plot('plots/hetero_egalitarian_gradient_basic_vs_ffa')


def sec2():
    basic = plot.PlotBasic('DynamicBasic100', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    ffa = plot.PlotBasic('DynamicBasic100FFA', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)

    sec2_egalitarian_gradient(basic, ffa)
    sec2_throughput(basic, ffa)
    # sec2_processing_variability()


def sec5_evaluation_microbenchmarks():
    # main example, inbox250
    basic = plot.PlotBasic('Scenario1', 'Scenario1', 180, verbose_logs=True, run_simulation=True, plot_all=False)
    basic.thesismain_micro_latency()
    basic.thesismain_micro_inbox_solidification_buffer()
    basic.thesismain_micro_throughput(annotation='Scenario1')
    healthor = plot.PlotV1('Scenario1V1', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=False)
    healthor.thesismain_micro_inbox_solidification_buffer()
    healthor.thesismain_micro_latency()
    healthor.thesismain_micro_available_processing()
    healthor.thesismain_micro_throughput()

    # Scenario2
    # main example, inbox250
    basic = plot.PlotBasic('Scenario2', 'Scenario1', 180, verbose_logs=True, run_simulation=True, plot_all=False)
    print_latency_percentile_node(basic, 'node[2]')
    print_latency_percentile_node(basic, 'node[6]')
    plot_latency_percentile(basic, '', plot=False)  # only print network latency percentile
    basic.thesismain_micro_latency()
    basic.thesismain_micro_inbox_solidification_buffer('Scenario2')
    basic.thesismain_micro_throughput('Scenario2')
    healthor = plot.PlotV1('Scenario2V1', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=False)
    print_latency_percentile_node(healthor, 'node[2]')
    print_latency_percentile_node(healthor, 'node[6]')
    plot_latency_percentile(healthor, '', plot=False)  # only print network latency percentile
    healthor.thesismain_micro_inbox_solidification_buffer()
    healthor.thesismain_micro_latency()
    healthor.thesismain_micro_available_processing(annotation='Scenario2')
    healthor.thesismain_micro_throughput()

    # inbox test series
    plot.PlotBasic('Scenario2Inbox100', 'Scenario1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    plot.PlotV1('Scenario2V1Inbox100', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    plot.PlotBasic('Scenario2Inbox500', 'Scenario1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    plot.PlotV1('Scenario2V1Inbox500', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    plot.PlotBasic('Scenario2Inbox1000', 'Scenario1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    plot.PlotV1('Scenario2V1Inbox1000', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    plot.PlotBasic('Scenario2Inbox2000', 'Scenario1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    plot.PlotV1('Scenario2V1Inbox2000', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=True)

    # outbox test series
    healthor = plot.PlotV1('Scenario2V1Outbox100', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    healthor.plot_buffers(to_node='node[4]')
    healthor.plot_buffers(to_node='node[6]')
    healthor = plot.PlotV1('Scenario2V1', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    healthor.plot_buffers(to_node='node[4]')
    healthor.plot_buffers(to_node='node[6]')
    healthor = plot.PlotV1('Scenario2V1Outbox500', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    healthor.plot_buffers(to_node='node[4]')
    healthor.plot_buffers(to_node='node[6]')
    healthor = plot.PlotV1('Scenario2V1Outbox1000', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    healthor.plot_buffers(to_node='node[4]')
    healthor.plot_buffers(to_node='node[6]')
    healthor = plot.PlotV1('Scenario2V1Outbox2000', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True, plot_all=True)
    healthor.plot_buffers(to_node='node[4]')
    healthor.plot_buffers(to_node='node[6]')


def sec5_throughput(basic, ffa, healthor, healthor_ffa=None, legend=True):
    init_plot()
    plot_throughput_mean(basic, MACRO_AIDED)
    plot_throughput_mean(ffa, MACRO_UNAIDED)
    plot_throughput_mean(healthor, MACRO_HEALTHOR)
    if healthor_ffa is not None:
        plot_throughput_mean(healthor_ffa, MACRO_HEALTHOR_UNAIDED)

    plt.ylabel('throughput (MPS)')
    plt.xlabel('time (s)')

    if legend:
        plt.legend()

    save_plot('plots/macro_throughput')


def sec5_egalitarian_gradient(basic, ffa, healthor, healthor_ffa=None, legend=True):
    init_plot()

    mean = basic.egalitarian_score(False, MACRO_AIDED)
    print('Egalitarian score basic: %.2f' % mean)
    mean = ffa.egalitarian_score(False, MACRO_UNAIDED)
    print('Egalitarian score FFA: %.2f' % mean)
    mean = healthor.egalitarian_score(False, MACRO_HEALTHOR)
    print('Egalitarian score Healthor: %.2f' % mean)
    if healthor_ffa is not None:
        mean = healthor_ffa.egalitarian_score(False, MACRO_HEALTHOR_UNAIDED)
        print('Egalitarian score Healthor FFA: %.2f' % mean)

    plt.ylabel(r'$c_{value}$')
    plt.xlabel('time (s)')

    if legend:
        plt.legend()

    save_plot('plots/macro_egalitarian_gradient')


def sec5_latency_percentiles(basic, ffa, healthor, healthor_ffa=None, legend=True):
    init_plot()

    plot_latency_percentile(basic, MACRO_AIDED)
    plot_latency_percentile(ffa, MACRO_UNAIDED)
    plot_latency_percentile(healthor, MACRO_HEALTHOR)
    if healthor_ffa is not None:
        plot_latency_percentile(healthor_ffa, MACRO_HEALTHOR_UNAIDED)

    plt.ylabel('cdf')
    plt.xlabel('time (s)')

    if legend:
        plt.legend()

    save_plot('plots/macro_latencies')


def sec5_evaluation_macrobenchmarks():
    plot.PlotBasic('MacroBasic100', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    plot.PlotBasic('MacroFFA100', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    plot.PlotV1('MacroHealthor100', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True, plot_all=False)

    plot.PlotBasic('MacroBasic500', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    plot.PlotBasic('MacroFFA500', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    plot.PlotV1('MacroHealthor500', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True, plot_all=False)

    plot.PlotBasic('MacroBasic1000', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    plot.PlotBasic('MacroFFA1000', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    plot.PlotV1('MacroHealthor1000', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True, plot_all=False)

    basic = plot.PlotBasic('MacroBasic2000', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    ffa = plot.PlotBasic('MacroFFA2000', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    healthor = plot.PlotV1('MacroHealthor2000', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    # generate plots for macrobenchmark section
    sec5_throughput(basic, ffa, healthor, legend=False)
    sec5_egalitarian_gradient(basic, ffa, healthor, legend=True)
    sec5_latency_percentiles(basic, ffa, healthor, legend=False)

    plot.PlotBasic('MacroBasic5000', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    plot.PlotBasic('MacroFFA5000', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    plot.PlotV1('MacroHealthor5000', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True, plot_all=False)
    return

    # this can be utilized once all the runs are done (e.g. to populate the results table)
    runs = [
        # 'MacroBasic100',
        # 'MacroFFA100',
        # 'MacroHealthor100',
        #
        # 'MacroBasic500',
        # 'MacroFFA500',
        # 'MacroHealthor500',
        #
        # 'MacroBasic1000',
        # 'MacroFFA1000',
        # 'MacroHealthor1000',
        #
        # 'MacroBasic2000',
        # 'MacroFFA2000',
        # 'MacroHealthor2000',

        # 'MacroBasic5000',
        # 'MacroFFA5000',
        # 'MacroHealthor5000',
    ]

    # for name in runs:
    #     print('Plotting %s...' % name)
    #     if name.startswith('MacroHealthor'):
    #         network = 'DynamicNetworkV1'
    #         p = plot.PlotV1(name, network, 180, verbose_logs=False, run_simulation=False, plot_all=False)
    #     else:
    #         network = 'DynamicNetworkBasic'
    #         p = plot.PlotBasic(name, network, 180, verbose_logs=False, run_simulation=False, plot_all=False)
    #
    #     # only print the necessary values for the table
    #     # plot_throughput_mean(p, '', plot=False)
    #     plot_latency_percentile(p, '', plot=False)
    #     # plot_egalitarian_gradient(p, '', plot=False)
    #     del p
    #     print('Plotting %s... done' % name)
    #     print('---------------------------------------')

    # calculate values for 5000 FFA (too big to run normally)
    def throughput_5000_special(path):
        df = pd.read_csv(path)

        all_nodes = list(filter(lambda x: x.startswith('node'), list(df)))
        print(len(all_nodes))

        # determine how many nodes were in sync and average
        total_count = None
        total_in_sync_nodes = None

        for node in all_nodes:
            count, division = np.histogram(df[node], bins=range(180))

            # sum up for throughput average
            if total_count is not None:
                total_count += count
                total_in_sync_nodes += (count > 0).astype(int)
            else:
                total_count = count
                total_in_sync_nodes = (count > 0).astype(int)

        # determine average
        # print(total_count[:-11])
        # print(total_in_sync_nodes[:-11])
        total_count = total_count[:-11] / total_in_sync_nodes[:-11]

        print('Throughput avg mean: %.2f' % np.mean(total_count))

    # throughput_5000_special('msgs-FFA-5000.csv')

    def latency_5000_special(path, percentile):
        df = pd.read_csv(path)
        all_nodes = list(filter(lambda x: x.startswith('node'), list(df)))
        print(len(all_nodes))

        difference_suffix = '_difference'
        msgs_count = df.shape[0]

        # determine which nodes CDF needs to be calculated
        nodes_in_sync = []

        # determine whether node was in sync (for most of the simulation)
        for node in all_nodes:
            msgs_node_missing = df[node].isnull().sum()
            if msgs_node_missing < msgs_count * 0.05:
                nodes_in_sync.append(node)

        print('nodes in sync count', len(nodes_in_sync))
        print('messages count', msgs_count)

        # calculate latency for every message on every in sync node
        # runtime: ~40s
        for node in nodes_in_sync:
            df[node + difference_suffix] = df[node].subtract(df['time'])
            print(node, 'done')

        #
        nodes_difference_columns = [x + difference_suffix for x in nodes_in_sync]
        percentile_name = 'percentile_' + str(percentile)
        percentile_name_difference = percentile_name + '_difference'
        percentile_df = pd.DataFrame(columns=[percentile_name_difference])
        percentile_df[percentile_name_difference] = df[nodes_difference_columns].quantile(percentile, axis=1)

        def calculate_cdf(d, n):
            node_difference = n + '_difference'
            stats_df = d.groupby([node_difference])[node_difference].agg('count').pipe(pd.DataFrame).rename(
                columns={node_difference: 'frequency'})
            stats_df['pdf'] = stats_df['frequency'] / sum(stats_df['frequency'])
            stats_df['cdf'] = stats_df['pdf'].cumsum()
            stats_df = stats_df.reset_index()
            return stats_df

        cdf_df = calculate_cdf(percentile_df, percentile_name)

        # print 0.95 percentile latency of CDF
        np_arr = cdf_df['cdf'].to_numpy()
        m_index = np.where(np_arr >= 0.95)[0][0]
        print('Percentile CDF %.2f 0.95th: %.2f' % (percentile, cdf_df[percentile_name_difference][m_index]))


def attack_analysis():
    # exceed sending rate experiment
    for name in ['ExceedSendingRateAttack', 'ExceedSendingRateAttackDisabledDefense']:
        h = plot.PlotV1(name, 'Scenario1V1', 60, verbose_logs=True, run_simulation=True, plot_all=False)
        h.plot_allowed_receiving_rate(print_node='node[2]', malicious_node='node[7]', annotation=name)

    # sending nothing to neighbor
    h = plot.PlotV1('SubceedSendingRateAttack', 'Scenario1V1', 60, verbose_logs=True, run_simulation=True, plot_all=False)
    h.plot_allowed_receiving_rate(print_node='node[2]', malicious_node='node[7]', annotation='SubceedSendingRateAttack')

    # low health
    h = plot.PlotV1('LowHealthAttack', 'Scenario1V1', 60, verbose_logs=True, run_simulation=True, plot_all=False)
    h.plot_outboxes(print_node='node[3]', annotation='LowHealthAttack')


def centralization_score_sensitivity():
    runs = [
        'MacroBasicSensitivity100T102',
        'MacroBasicSensitivity100T104',
        'MacroBasicSensitivity100T98',
        'MacroBasicSensitivity100T96',
        'MacroBasicSensitivity100T94',
        'MacroBasicSensitivity100T92',

        'MacroHealthorSensitivity100T96',
        'MacroHealthorSensitivity100T98',
        'MacroHealthorSensitivity100T102',
        'MacroHealthorSensitivity100T104',
        'MacroHealthorSensitivity100T103',
        'MacroHealthorSensitivity100T106',
        'MacroHealthorSensitivity100T108',
        'MacroHealthorSensitivity100T110',
        'MacroHealthorSensitivity100T112',
        'MacroHealthorSensitivity100T114',
        'MacroHealthorSensitivity100T116',
        'MacroHealthorSensitivity100T118',
        'MacroHealthorSensitivity100T120',
        'MacroHealthorSensitivity100T122',
        'MacroHealthorSensitivity100T124',
        'MacroHealthorSensitivity100T126',
        'MacroHealthorSensitivity100T140',
        'MacroHealthorSensitivity100T160',
        'MacroHealthorSensitivity100T180',
        'MacroHealthorSensitivity100T200',
    ]

    for name in runs:
        print('Plotting %s...' % name)
        if name.startswith('MacroHealthor'):
            network = 'DynamicNetworkV1'
            p = plot.PlotV1(name, network, 180, verbose_logs=False, run_simulation=True, plot_all=True)
        else:
            network = 'DynamicNetworkBasic'
            p = plot.PlotBasic(name, network, 180, verbose_logs=False, run_simulation=True, plot_all=True)

        # only print the necessary values for the table
        plot_throughput_mean(p, '', plot=False)
        plot_latency_percentile(p, '', plot=False)
        plot_egalitarian_gradient(p, '', plot=False)
        del p
        print('Plotting %s... done' % name)
        print('---------------------------------------')


def plot_centralization_score_sensitivity():
    aided_cs = [
        0.00,
        0.02,
        0.03,
        0.04,
        0.10,
        0.44,
    ]
    aided_throughput = [
        93.04,
        94.99,
        96.87,
        98.24,
        99.43,
        99.50,
    ]
    aided_latency = [
        1.27,
        1.1,
        1.7,
        1.93,
        5.58,
        10.05,
    ]

    healthor_cs = [
        0.00,
        0.02,
        0.03,
        0.05,
        0.06,
        0.09,
        0.10,
        0.14,
        0.15,
        0.18,
        0.24,
        0.30,
        0.36,
    ]
    healthor_throughput = [
        98.68,
        101.10,
        103.68,
        108.29,
        109.09,
        114.14,
        115.13,
        116.85,
        119.93,
        121.82,
        131.13,
        156.31,
        168.15,
    ]
    healthor_latency = [
        0.68,
        0.73,
        0.75,
        0.91,
        0.93,
        0.77,
        0.74,
        0.74,
        0.75,
        0.85,
        0.72,
        0.75,
        1.06,
    ]

    # plot to throughput data
    init_plot((4, 4))

    plt.plot(aided_cs, aided_throughput, label=MACRO_AIDED, linewidth=1.3, markevery=0.1)
    plt.plot(healthor_cs, healthor_throughput, label=MACRO_HEALTHOR, linewidth=1.3, color='C2', marker='<', markevery=0.1)

    plt.xlim(right=0.48)

    plt.ylabel('throughput (MPS)')
    plt.xlabel('centralization score')

    # plt.legend()

    save_plot('plots/sensitivity_throughput')

    # plot to throughput data
    init_plot((4, 4))

    plt.plot(aided_cs, aided_latency, label=MACRO_AIDED, linewidth=1.3, markevery=0.1)
    plt.plot(healthor_cs, healthor_latency, label=MACRO_HEALTHOR, linewidth=1.3, color='C2', marker='<', markevery=0.1)

    plt.xlim(right=0.48)

    plt.ylabel('95 percentile latency')
    plt.xlabel('centralization score')

    plt.legend()

    save_plot('plots/sensitivity_latency')


def utilization():
    # utilization N=100
    p = plot.PlotV1('MacroHealthorUtilizationN100T106', 'DynamicNetworkV1', 180, verbose_logs=False,
                    run_simulation=True, plot_all=False)
    # only print the necessary values for the table
    plot_throughput_mean(p, '', plot=False)
    plot_latency_percentile(p, '', plot=False)
    plot_egalitarian_gradient(p, '', plot=False)

    runs = [
        'MacroHealthorUtilizationN500T128',
        'MacroHealthorUtilizationN500T126',
        'MacroHealthorUtilizationN500T124',

        'MacroHealthorUtilizationN1000T116',
        'MacroHealthorUtilizationN1000T114',
        'MacroHealthorUtilizationN1000T112',

        'MacroHealthorUtilizationN2000T110',
        'MacroHealthorUtilizationN2000T108',
        'MacroHealthorUtilizationN2000T106',
    ]

    for name in runs:
        print('Plotting %s...' % name)
        if name.startswith('MacroHealthor'):
            network = 'DynamicNetworkV1'
            p = plot.PlotV1(name, network, 180, verbose_logs=False, run_simulation=True, plot_all=True)
        else:
            network = 'DynamicNetworkBasic'
            p = plot.PlotBasic(name, network, 180, verbose_logs=False, run_simulation=True, plot_all=True)

        # only print the necessary values for the table
        plot_throughput_mean(p, '', plot=False)
        plot_latency_percentile(p, '', plot=False)
        plot_egalitarian_gradient(p, '', plot=False)
        del p
        print('Plotting %s... done' % name)
        print('---------------------------------------')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Simulation running and plotting framework of Healthor. Please make sure that OMNeT++ is set up correctly as described here: https://github.com/jonastheis/healthor.')
    parser.add_argument("--microbenchmarks", help="run simulation series and plot results of microbenchmarks with 2 different 10 node networks", action='store_true')
    parser.add_argument("--macrobenchmarks", help="run simulation series and plot results  of macrobenchmarks", action='store_true')
    parser.add_argument("--attack-analysis", help="run simulations and plot results of attacks", action='store_true')
    parser.add_argument("--centralization-sensitivity-analysis", help="run and plot results of centralization score sensitivity analysis", action='store_true')
    parser.add_argument("--initial-experiment", help="run simulation and plot results of the initial experiment aided vs unaided", action='store_true')
    parser.add_argument("--processing-variability", help="plot processing variability of various cloud hosting provider", action='store_true')
    args = parser.parse_args()

    # exit and show help if no simulation is specified
    if True not in args.__dict__.values():
        print("Please call with at least one simulation to run and plot.\n")
        parser.print_help()
        sys.exit(1)

    print(args)

    if args.microbenchmarks:
        print("Running microbenchmarks...")
        sec5_evaluation_microbenchmarks()
        print("Running microbenchmarks... done")

    if args.macrobenchmarks:
        print("Running macrobenchmarks...")
        sec5_evaluation_macrobenchmarks()
        print("Running macrobenchmarks... done")

    if args.attack_analysis:
        print("Running attack analysis...")
        attack_analysis()
        print("Running attack analysis... done")

    if args.centralization_sensitivity_analysis:
        print("Running centralization score sensitivity analysis...")
        centralization_score_sensitivity()
        plot_centralization_score_sensitivity()
        utilization()
        print("Running centralization score sensitivity analysis... done")

    if args.initial_experiment:
        print("Running initial experiment aided vs unaided...")
        sec2()
        print("Running initial experiment aided vs unaided... done")

    if args.processing_variability:
        print("Plotting processing variability of various cloud hosting provider...")
        sec2_processing_variability()
        print("Plotting processing variability of various cloud hosting provider... done")

