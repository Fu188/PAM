double Q19(maps m, const int quantity_low_, const int quantity_high_, const int price_low_, const int price_high_, const char **data_) {
    const int warehouse_[3][3] = {{1,2,3}, {1,2,4}, {1,5,3}};
    using AddD = Add<double>;

    auto streq = [](const char *s1, const char *s2) -> bool {
        return strcmp(s1, s2) == 0;
    };

    auto is_in = [&](const int s1, const int *list, int n) -> bool {
        for (int i = 0; i < n; i++)
            if (s1 == list[i]) return true;
        return false;
    };

    auto f = [&](item_to_supp_map::E &e) -> double {

        Item p = e.second.first;
        item_supp_and_orderline_map &si = e.second.second;
        char* t = p.i_data();

        auto block = [&](const int quantity_low, const int quantity_high, const int price_low, const int price_high, const char *data, const int *warehouse) -> double {
            int typelen = strlen(data);
            if ((p.i_price >= price_low && p.i_price <= price_high)
                && (strcmp(t + strlen(t)- typelen, data) == 0)) {
                auto g = [&](item_supp_and_orderline_map::E &s) -> double {
                    ol_map &ol = s.second.second;
                    auto h = [&](OrderLine &ol) -> double {
                        if ((ol.ol_quantity >= quantity_low) &&
                            (ol.ol_quantity <= quantity_high) &&
                            (is_in(ol.ol_w_id, warehouse, 3)))
                            return ol.ol_amount;
                        else return 0.0;
                    };
                    return ol_map::map_reduce(ol, h, AddD());
                };
                return item_supp_and_orderline_map::map_reduce(si, g, AddD());
            } else return 0.0;
        };

        double total = 0.0;
        for (int i = 0; i < 3; i++)
            total += block(quantity_low_, quantity_high_, price_low_, price_high_, data_[i], warehouse_[i]);
        return total;
    };

    double result = item_to_supp_map::map_reduce(m.ism2, f, AddD(), 1);
    return result;
}

double Q19time(maps m, bool verbose) {
    timer t_q19;
    t_q19.start();
    const int quantity_low = 1;
    const int quantity_high = 10;
    const int price_low = 1;
    const int price_high = 400000;
    const char *data[3] = {"a", "b", "c"};

    double result = Q19(m, quantity_low, quantity_high, price_low, price_high, data);

    if (verbose) {
        cout << "Q19:" << endl
             << result << endl;
    }
    double ret_tm = t_q19.stop();
    if (query_out) cout << "Q19 : " << ret_tm << endl;
    return ret_tm;
}  
  
