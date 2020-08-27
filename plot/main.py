import plot

if __name__ == '__main__':

    # plot.PlotBasic('ShowProblem', 'BasicNetwork', 90)
    # plot.PlotV1('ShowProblemV1', 'V1Network', 90)

    plot.PlotBasic('Scenario1', 'Scenario1', 90)
    plot.PlotV1('Scenario1V1', 'Scenario1V1', 90)

    # s = plot.Serial20('ShowProblem', 'BasicNetwork', 90)
    # s.plot_issue()
    #
    # s = plot.Serial20('Scenario1', 'Scenario1', 90)
    # s.plot_inboxes()
    # s.plot_throughput()
    # s.plot_available_processing()