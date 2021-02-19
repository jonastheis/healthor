import plot


def test_series():
    plot.PlotBasic('BasicInbox100', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    plot.PlotBasic('BasicInbox100FFA', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox100Outbox50', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox100Outbox100', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox100Outbox200', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)

    plot.PlotBasic('BasicInbox500', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    plot.PlotBasic('BasicInbox500FFA', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox500Outbox100', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox500Outbox500', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox500Outbox1000', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)

    plot.PlotBasic('BasicInbox100Generation90', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    plot.PlotBasic('BasicInbox100Generation90FFA', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox100Outbox50Generation90', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox100Outbox100Generation90', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox100Outbox200Generation90', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)

    plot.PlotBasic('BasicInbox500Generation90', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    plot.PlotBasic('BasicInbox500Generation90FFA', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox500Outbox100Generation90', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox500Outbox500Generation90', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)
    plot.PlotV1('V1Inbox500Outbox1000Generation90', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)


def serial20():
    s = plot.Serial20('ShowProblem', 'BasicNetwork', 90)
    s.plot_issue()

    s = plot.Serial20('Scenario1', 'Scenario1', 90)
    s.plot_inboxes()
    s.plot_throughput()
    s.plot_available_processing()


def attack_analysis():
    # exceed sending rate experiment
    # for name in ['ExceedSendingRateAttack', 'ExceedSendingRateAttackDisabledDefense']:
    #     h = plot.PlotV1(name, 'Scenario1V1', 60, verbose_logs=True, run_simulation=True, plot_all=False)
    #     h.plot_allowedReceivingRate(print_node='node[2]', malicious_node='node[7]')
    #     h.plot_buffers()
    #     h.plot_sending_rate()
    #     h.plot_throughput()
    #     h.egalitarian_score()

    # sending nothing to neighbor
    # h = plot.PlotV1('SubceedSendingRateAttack', 'Scenario1V1', 60, verbose_logs=True, run_simulation=True, plot_all=False)
    # h.plot_allowedReceivingRate(print_node='node[2]', malicious_node='node[7]')
    # h.plot_buffers()
    # h.plot_sending_rate()
    # h.plot_throughput()
    # h.egalitarian_score()

    # low health
    h = plot.PlotV1('LowHealthAttack', 'Scenario1V1', 60, verbose_logs=True, run_simulation=True, plot_all=False)
    h.plot_outboxes(print_node='node[0]')
    h.plot_outboxes(print_node='node[2]')
    h.plot_outboxes(print_node='node[3]')
    h.plot_outboxes(print_node='node[8]')
    h.plot_outboxes(print_node=None)
    h.plot_sending_rate()
    h.plot_throughput()
    h.egalitarian_score()

if __name__ == '__main__':

    # plot.PlotBasic('DynamicBasicTesting', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    # plot.PlotV1('DynamicV1Testing', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)

    # plot.PlotBasic('ShowProblem', 'BasicNetwork', 719)
    # plot.PlotV1('ShowProblemV1', 'V1Network', 90)

    # plot.PlotBasic('Scenario1', 'Scenario1', 180, verbose_logs=True, run_simulation=True)
    # plot.PlotBasic('Scenario1FFA', 'Scenario1', 180, verbose_logs=True, run_simulation=True)
    # plot.PlotV1('Scenario1V1', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True)

    # plot.PlotBasic('DynamicBasic100', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=False)
    # plot.PlotBasic('DynamicBasic100FFA', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=False)
    # plot.PlotV1('DynamicV1100', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)

    # plot.PlotBasic('DynamicBasic1000', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    # plot.PlotBasic('DynamicBasic1000FFA', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    # plot.PlotV1('DynamicV11000', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)

    # plot.PlotBasic('DynamicBasic2000', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    # plot.PlotBasic('DynamicBasic2000FFA', 'DynamicNetworkBasic', 180, verbose_logs=False, run_simulation=True)
    # plot.PlotV1('DynamicV12000', 'DynamicNetworkV1', 180, verbose_logs=False, run_simulation=True)

    # test_series()

    # plot.PlotBasic('Scenario2x', 'Scenario1', 180, verbose_logs=True, run_simulation=True)
    # plot.PlotBasic('Scenario2xFFA', 'Scenario1', 180, verbose_logs=True, run_simulation=True)
    # plot.PlotV1('Scenario2xV1', 'Scenario1V1', 180, verbose_logs=True, run_simulation=True)

    attack_analysis()