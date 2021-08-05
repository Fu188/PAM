double Q6(maps m, const char *start, const char *end, int quantity_begin, int quantity_end) {
    delivery_map dm = m.dm;
    delivery_map delivery_range = delivery_map::range(dm, Date(start), Date(end));

    auto delivery_f = [&](delivery_map::E &e) -> double {
        auto ol_f = [=](ol_map::E &ol) -> double {
            int quant = ol.ol_quantity;
            if ((quant > quantity_begin) && (quant < quantity_end))
                return ol.ol_amount;
            else return 0.0;
        };
        return ol_map::map_reduce(e.second, ol_f, Add<double>());
    };

    return delivery_map::map_reduce(delivery_range, delivery_f, Add<double>(), 1);
}

double Q6time(maps m, bool verbose) {
    timer t;
    t.start();
    const char start[] = "1999-01-01";
    const char end[] = "2020-01-01";
    const int quantity_begin = 1;
    const int quantity_end = 100000;

    double result = Q6(m, start, end, quantity_begin, quantity_end);
    double ret_tm = t.stop();
    cout << "Q6 : " << ret_tm << endl;

    if (verbose) cout << "Q6:" << endl << result << endl;
    return ret_tm;
}

