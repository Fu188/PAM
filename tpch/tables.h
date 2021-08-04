int CUSTOMER_NUM, ORDER_NUM, ORDERLINE_NUM, ITEM_NUM, STOCK_NUM, NATION_NUM, SUPPLIER_NUM, REGION_NUM;
auto sep = [] (char c) -> bool { return c == '|'; };
int total_nation = 25;

template <class S>
sequence<S> read_and_parse(string filename, bool verbose) {
  csv_data d = readCSV(filename, sep, verbose);
  auto parseItem = [&] (size_t i) { return S(d.lines[i]);};
  auto x = sequence<S>(d.lines.size(), parseItem);
  return x;
}

struct ol_entry {
      using key_t = OrderLine;
      static bool comp(const key_t&a, const key_t& b) {
          return ((a.ol_o_id < b.ol_o_id) ||
                  (a.ol_o_id == b.ol_o_id && a.ol_d_id < b.ol_d_id) ||
                  (a.ol_o_id == b.ol_o_id && a.ol_d_id == b.ol_d_id && a.ol_w_id < b.ol_w_id) ||
                  (a.ol_o_id == b.ol_o_id && a.ol_d_id == b.ol_d_id && a.ol_w_id == b.ol_w_id && a.ol_number < b.ol_number));
      }
  };

using ol_map = pam_set<ol_entry>;
using delivery_map = date_map<ol_map>;
using order_map = keyed_map<pair<Order,ol_map>>;
using customer_map = keyed_map<pair<Customer,order_map>>;
using o_order_map = date_map<order_map>;
using just_order_map = keyed_map<Order>;

using item_supp_map = paired_key_map<Stock>;
using item_supp_and_orderline_map = paired_key_map<pair<Stock, ol_map>>;
using item_to_supp_map = keyed_map<pair<Item, item_supp_and_orderline_map>>;
using supp_to_item_map = keyed_map<pair<Supplier, item_supp_and_orderline_map>>;

struct maps {
    order_map om;
    delivery_map dm;
    customer_map cm;
    o_order_map oom;
    item_to_supp_map ism2;
    supp_to_item_map sim2;

    maps() {}
    maps(const maps& m) {
        om = m.om;
        dm = m.dm;
        cm = m.cm;
        oom = m.oom;
        ism2 = m.ism2;
        sim2 = m.sim2;
    }

    maps& operator = (const maps& m) {
        if (this != &m) {
            om = m.om;
            dm = m.dm;
            cm = m.cm;
            oom = m.oom;
            ism2 = m.ism2;
            sim2 = m.sim2;
        }
        return *this;
    }

    void clear() {
        om.clear();
        dm.clear();
        cm.clear();
        oom.clear();
        ism2.clear();
        sim2.clear();
    }
};

void memory_stats() {
  cout << "orderline map:" << endl;
  ol_map::GC::print_stats();
  cout << "order map:" << endl;
  order_map::GC::print_stats();
  cout << "customer map:" << endl;
  customer_map::GC::print_stats();
  cout << "orderdate map:" << endl;
  o_order_map::GC::print_stats();
  cout << "deliverydate map:" << endl;
  delivery_map::GC::print_stats();
  cout << "stock and item map:" << endl;
  item_supp_and_orderline_map::GC::print_stats();
  cout << "item map:" << endl;
  item_to_supp_map::GC::print_stats();
  cout << "supplier map:" << endl;
  supp_to_item_map::GC::print_stats();
}

struct arrays_and_temps {
  Supplier* all_supp;
  Item* all_item;
  item_supp_map ism;
} static_data;

maps make_maps_test(string dirname, bool verbose) {
    maps m;

    timer tx; tx.start();
    cout << sizeof(Customer) << endl;
    cout << sizeof(Order) << endl;
    cout << sizeof(Item) << endl;
    cout << sizeof(Stock) << endl;
    cout << sizeof(OrderLine) << endl;
    cout << sizeof(Nation) << endl;
    cout << sizeof(Supplier) << endl;

    /*****************
    ** orderline
    *****************/
    cout << "-- orderline" << endl;
    string ol_f = dirname;
    string orderline_fname = ol_f.append("ORDERLINE.tbl");
    sequence<OrderLine> orderlines = read_and_parse<OrderLine>(orderline_fname, verbose);
    ORDERLINE_NUM = orderlines.size();
    if (verbose) {nextTime("parse line items");}

    // first keyed on delivery date
    m.dm = secondary_index<delivery_map>(orderlines, [] (OrderLine ol) {
        return ol.ol_delivery_d;
    });
    if (verbose) nextTime("delivery_date -> orderline index");

    {
        /********************
        ** order
        ********************/
        cout << "-- order" << endl;
        string od_f = dirname;
        string order_fname = od_f.append("ORDER.tbl");
        sequence<Order> orders = read_and_parse<Order>(order_fname, verbose);
        ORDER_NUM = orders.size();
        if (verbose) {nextTime("parse order");}

        using ordermap = keyed_map<Order>;
        using co_ol_map = paired_key_map<ol_map>;
        {
            auto order_primary = primary_index<ordermap>(orders, [&] (Order o) {
                return o.o_pkey;
            });

            auto comap = secondary_index<co_ol_map>(orderlines, [&] (OrderLine ol) {
                return make_pair((*(order_primary.find(ol.ol_pkey))).o_c_pkey,
                                 ol.ol_pkey);
            });

            auto get_key = [&] (Order& o) -> dkey_t {
                return o.o_pkey;
            };

            auto get_val = [&] (Order& o) -> ol_map {
                maybe<ol_map> x = comap.find(make_pair(o.o_c_pkey,o.o_pkey));
                return x ? *x : ol_map();
            };

            m.om = make_paired_index<order_map>(orders, get_key, get_val);
        }
        ordermap::GC::finish(); // return memory
        co_ol_map::GC::finish(); // return memory

        auto get_o_date =
                m.oom = secondary_index<o_order_map>(m.om, [] (order_map::E e) -> Date {
                    return e.second.first.o_entry_d;
                });
        if (verbose) {nextTime("orderkey -> <order,orderline> index");}


        /********************
        ** customer
        ********************/
        cout << "-- customer" << endl;
        string c_f = dirname;
        string customer_fname = c_f.append("CUSTOMER.tbl");
        sequence<Customer> customers = read_and_parse<Customer>(customer_fname, verbose);
        CUSTOMER_NUM = customers.size();
        if (verbose) {nextTime("parse customer");}

        using cust_map = keyed_map<order_map>;
        cust_map cmap = secondary_index<cust_map>(m.om, [] (order_map::E e) {
            return ((e.second).first).o_c_pkey;
        });
        if (verbose) nextTime("custkey -> orderkey -> <order,lineitem> index");

        auto a = primary_index<keyed_map<Customer>>(customers, [&] (Customer c) {
            return c.c_pkey;
        });
        m.cm = paired_index<customer_map>(a, cmap);
        cmap = cust_map();
        cust_map::GC::finish();
        if (verbose) {nextTime("custkey -> <customer, orderkey -> <order, lineitem>> index");}
    }

    {
        /********************
        ** Stock
        ********************/
        cout << "-- stock" << endl;
        string s_f = dirname;
        string stock_fname = s_f.append("STOCK.tbl");
        sequence<Stock> stocks = read_and_parse<Stock>(stock_fname, verbose);
        STOCK_NUM = stocks.size();
        if (verbose) {nextTime("parse stocks");}

        static_data.ism = primary_index<item_supp_map>(stocks, [&] (Stock s) {
            return make_pair(s.s_i_id, s.s_su_suppkey);
        });
        if (verbose) {nextTime("<itemkey,suppkey> -> stock index");}

        // make map from stock to orderline
        using st_ol_map = paired_key_map<ol_map>;
        st_ol_map amap = secondary_index<st_ol_map>(orderlines, [] (OrderLine ol) {
            return make_pair(ol.ol_i_id, ol.ol_supply_w_id);
        });
        if (verbose) {nextTime("<itemkey,supp_w_key> -> orderline index");}
        item_supp_and_orderline_map isom = paired_index<item_supp_and_orderline_map>(static_data.ism, amap);
        if (verbose) {nextTime("<itemkey,supp_w_key> -> (stock,orderline index)");}

        using tmp_map = keyed_map<item_supp_and_orderline_map>;

        {
            /********************
            ** Supplier
            ********************/
            cout << "-- supplier" << endl;
            string su_f = dirname;
            string supplier_fname = su_f.append("SUPPLIER.tbl");
            sequence<Supplier> suppliers = read_and_parse<Supplier>(supplier_fname, verbose);
            SUPPLIER_NUM = suppliers.size();
            if (verbose) {nextTime("parse suppliers");}

            static_data.all_supp = new Supplier[SUPPLIER_NUM+1];
            parallel_for (0, suppliers.size(), [&] (size_t i) {
                static_data.all_supp[suppliers[i].su_suppkey] = suppliers[i];
            });

            cout << "suppkey->(supplier, stock->orderline)" << endl;
            using supp_map = keyed_map<Supplier>;
            supp_map supp_primary = primary_index<supp_map>(suppliers, [&] (Supplier p) {return p.su_suppkey;});
            tmp_map supp_other = secondary_index<tmp_map>(isom, [] (item_supp_and_orderline_map::E e) {
                        return e.second.first.s_su_suppkey;
            });
            m.sim2 = paired_index<supp_to_item_map>(supp_primary, supp_other);
            if (verbose) cout << "sim2 size: " << m.sim2.size() << endl;
        }

        {
            /********************
            ** Item
            ********************/
            cout << "-- item" << endl;
            string i_f = dirname;
            string item_fname = i_f.append("ITEM.tbl");
            sequence<Item> items = read_and_parse<Item>(item_fname, verbose);
            ITEM_NUM = items.size();
            if (verbose) {nextTime("parse items");}

            static_data.all_item = new Item[ITEM_NUM+1];
            parallel_for (0, items.size(), [&] (size_t i) {
                static_data.all_item[items[i].i_id] = items[i];
            });
            if (verbose) {nextTime("all items stored");}

            cout << "i_id->(item,stock->orderline)" << endl;
            using item_map = keyed_map<Item>;
            item_map item_primary = primary_index<item_map>(items, [&] (Item i) {return i.i_id;});
            using tmp_map = keyed_map<item_supp_and_orderline_map>;
            tmp_map item_other = secondary_index<tmp_map>(isom, [] (item_supp_and_orderline_map::E e) {
                return e.second.first.s_i_id;
            });

            m.ism2 = paired_index<item_to_supp_map>(item_primary, item_other);
            if (verbose) cout << "ism2 size: " << m.ism2.size() << endl;
        }
    }

    {
        /********************
        ** Nation
        ********************/
        cout << "-- nation" << endl;
        string n_f = dirname;
        string nation_fname = n_f.append("NATION.tbl");
        nations = read_and_parse<Nation>(nation_fname, verbose);
        NATION_NUM = nations.size();
        if (verbose) {nextTime("parse nations");}
    }

    return m;
}

//maps make_maps(string dirname, bool verbose) {
//  maps m;
//  string lf = dirname;
//
//  timer tx; tx.start();
//  cout << sizeof(Lineitem) << endl;
//  cout << sizeof(Orders) << endl;
//  cout << sizeof(Customer) << endl;
//  cout << sizeof(Part_supp) << endl;
//  cout << sizeof(Part) << endl;
//  cout << sizeof(Supplier) << endl;
//
//  // lineitems
//  cout << "-- lineitems" << endl;
//
//  // first keyed on shipdate
//  string lineitem_fname = lf.append("lineitem.tbl");
//  sequence<Lineitem> items = read_and_parse<Lineitem>(lineitem_fname, verbose);
//  cout << items.size() << endl;
//  if (verbose) {nextTime("parse line items");}
//
//  m.sm = secondary_index<ship_map>(items, [] (Lineitem l) {
//      return l.s_date;});
//  if (verbose) nextTime("ship_date -> lineitem index");
//
//  m.rm = secondary_index<receipt_map>(items, [] (Lineitem l) {
//      return l.r_date;});
//  if (verbose) nextTime("receipt_date -> lineitem index");
//
//  LINEITEM_NUM = items.size();
//
//  // orders and customers
//  {
//    if (verbose) cout << endl;
//    cout << "-- orders" << endl;
//
//    string of = dirname;
//    string orders_fname = of.append("orders.tbl");
//    sequence<Orders> orders = read_and_parse<Orders>(orders_fname, verbose);
//	ORDER_NUM = orders.size();
//	CURR_ORDER = orders[ORDER_NUM-1].orderkey;
//    if (verbose) nextTime("parse orders");
//
//    using ordermap = keyed_map<Orders>;
//    using co_li_map = paired_key_map<li_map>;
//    {
//      auto orders_primary = primary_index<ordermap>(orders, [&] (Orders o) {
//	  return o.orderkey;});
//
//      auto comap = secondary_index<co_li_map>(items, [&] (Lineitem l) {
//	  return make_pair((*(orders_primary.find(l.orderkey))).custkey,
//			   l.orderkey);});
//
//      auto get_key = [&] (Orders& o) -> dkey_t {
//	return o.orderkey;};
//      auto get_val = [&] (Orders& o) -> li_map {
//	maybe<li_map> x = comap.find(make_pair(o.custkey,o.orderkey));
//	return x ? *x : li_map();
//      };
//      m.om = make_paired_index<order_map>(orders, get_key, get_val);
//    }
//    ordermap::GC::finish(); // return memory
//    co_li_map::GC::finish(); // return memory
//
//    auto get_o_date =
//    m.oom = secondary_index<o_order_map>(m.om, [] (order_map::E e) -> Date {
//	return e.second.first.orderdate;});
//
//    if (verbose) {nextTime("orderkey -> <order,lineitem> index");}
//
//    if (verbose) cout << endl;
//    cout << "-- customers" << endl;
//
//    using cust_map = keyed_map<order_map>;
//    cust_map cmap = secondary_index<cust_map>(m.om, [] (order_map::E e) {
//	return ((e.second).first).custkey;});
//    if (verbose) nextTime("custkey -> orderkey -> <order,lineitem> index");
//
//    string cf = dirname;
//    string customers_fname = cf.append("customers.tbl");
//    sequence<Customer> customers
//      = read_and_parse<Customer>(customers_fname, verbose);
//    CUSTOMER_NUM = customers.size();
//    if (verbose) nextTime("parse customers");
//
//    auto a = primary_index<keyed_map<Customer>>(customers, [&] (Customer c) {
//	return c.custkey;});
//
//    m.cm = paired_index<customer_map>(a, cmap);
//    cmap = cust_map();
//    cust_map::GC::finish();
//
//    if (verbose) {nextTime("custkey -> <customer, orderkey -> <order, lineitem>> index");}
//
//  }
//
//
//  //part supp and part_supp
//  {
//    //supp and part_supp
//    if (verbose) cout << endl;
//    cout << "-- part_suppliers" << endl;
//    string of = dirname;
//    string partsupp_fname = of.append("partsupp.tbl");
//    sequence<Part_supp> partsupp = read_and_parse<Part_supp>(partsupp_fname,
//							     verbose);
//    PARTSUPP_NUM = partsupp.size();
//    if (verbose) nextTime("parse partsupp");
//    if (verbose) cout << "partsup size: " << PARTSUPP_NUM << endl;
//
//    static_data.psm
//      = primary_index<part_supp_map>(partsupp, [&] (Part_supp ps) {
//	  return make_pair(ps.partkey, ps. suppkey);});
//
//    if (verbose) {nextTime("<partkey,suppkey> -> partsupp index");}
//    PARTSUPP_NUM = partsupp.size();
//
//    // make map from partsupp to item
//    using ps_item_map = paired_key_map<li_map>;
//    ps_item_map amap = secondary_index<ps_item_map>(items, [] (Lineitem l) {
//	return make_pair(l.partkey,l.suppkey);});
//    if (verbose) {nextTime("<partkey,suppkey> -> lineitem index");}
//
//    part_supp_and_item_map sptmp =
//      paired_index<part_supp_and_item_map>(static_data.psm, amap);
//    if (verbose) {nextTime("<partkey,suppkey> -> (partsupp,lineitem index)");}
//
//    using tmp_map = keyed_map<part_supp_and_item_map>;
//
//    //parts
//    {
//      if (verbose) cout << endl;
//      cout << "-- parts" << endl;
//      of = dirname;
//      string parts_fname = of.append("part.tbl");
//      sequence<Part> parts = read_and_parse<Part>(parts_fname, verbose);
//      PART_NUM = parts.size();
//      if (verbose) nextTime("parse parts");
//
//      static_data.all_part = new Part[PART_NUM+1];
//      parallel_for (0, parts.size(), [&] (size_t i) {
//	  static_data.all_part[parts[i].partkey] = parts[i];
//	});
//      if (verbose) {nextTime("all parts stored");}
//
//      cout << "partkey->(part,partsupp->lineitem)" << endl;
//      using part_map = keyed_map<Part>;
//      part_map part_primary
//	= primary_index<part_map>(parts, [&] (Part p) {return p.partkey;});
//      tmp_map part_other
//	= secondary_index<tmp_map>(sptmp, [] (part_supp_and_item_map::E e) {
//	    return e.second.first.partkey;});
//      m.psm2 = paired_index<part_to_supp_map>(part_primary, part_other);
//      if (verbose) cout << "psm2 size: " << m.psm2.size() << endl;
//    }
//
//    //suppliers
//    {
//      if (verbose) cout << endl;
//      cout << "-- suppliers" << endl;
//      of = dirname;
//      string supp_fname = of.append("supplier.tbl");
//      sequence<Supplier> supp = read_and_parse<Supplier>(supp_fname, verbose);
//      if (verbose) nextTime("parse suppliers");
//
//      SUPPLIER_NUM = supp.size();
//      static_data.all_supp = new Supplier[SUPPLIER_NUM+1];
//      parallel_for (0, supp.size(), [&] (size_t i) {
//	  static_data.all_supp[supp[i].suppkey] = supp[i];
//	});
//
//      cout << "suppkey->(supplier,partsupp->lineitem)" << endl;
//      using supp_map = keyed_map<Supplier>;
//      supp_map supp_primary
//	= primary_index<supp_map>(supp, [&] (Supplier p) {return p.suppkey;});
//      tmp_map supp_other
//	= secondary_index<tmp_map>(sptmp, [] (part_supp_and_item_map::E e) {
//	    return e.second.first.suppkey;});
//      m.spm2 = paired_index<supp_to_part_map>(supp_primary, supp_other);
//      if (verbose) cout << "spm2 size: " << m.spm2.size() << endl;
//    }
//  }
//
//  //nations
//  if (verbose) cout << endl;
//  cout << "-- nations" << endl;  string of = dirname;
//
//  string nation_fname = of.append("nation.tbl");
//  nations = read_and_parse<Nations>(nation_fname, verbose);
//  m.version = 0;
//  cout << "build all index in time: " << tx.stop() << endl;
//
//  return m;
//}

//auto revenue = [] (li_map::E& l) -> float {
//  return l.e_price * (1.0 - l.discount.val());
//};

auto plus_float = [] (float a, float b) -> float {return float(a + b);};
auto plus_double = [] (double a, double b) -> double {return double(a + b);};
auto plus_size_t = [] (size_t a, size_t b) {return a + b;};
auto plus_int = [] (int a, int b) {return a + b;};
template <class P>
P plus_pair(P a, P b) {
  return make_pair(a.first+b.first, a.second+b.second);
}

template <class T>
T plus_f(T a, T b) {return a + b;}

template <class T>
T max_f(T a, T b) {return std::max(a,b);}

template <class T>
T min_f(T a, T b) {return std::min(a,b);}

auto min_float = [] (float a, float b) -> float {return std::min(a,b);};

