[R345]L2 在集合竞价阶段的数据
【深圳】
从9：15到9：25开始能订阅到逐笔委托行情。9:15开始发OrderDetail和Transaction，因为9:25以前不会有成交，所以此时收到的Transaction只有撤单；9:25开始发成交。
【上海】
匹配成交的逐笔委托行情从9:25开始推送。9:25：先发Transaction，然后在OrderDetail中发出没有成交的报单和撤单。

注意：沪深两市的撤单是通过不同接口推送的，深市走Transaction接口，沪市走OrderDetail接口。
-----------------------------------------------------------------------------------------------------------------------------------

[R343]lv2逐笔数据丢包判断方法：
（1）每个委托通道的序号（SubSeq）都应该连续，并从 1 开始。如果程序检查到某个委托通道的序号不连续，则可以判断为丢包。因上海NGTS、上海XTS、深圳三个领域各自独立，需要准备三个序列分别判断丢包情况。
（2）对于没有启用NGTS的上海老版逐笔环境，由于委托和成交各自独立编号，还需要将委托和成交队列分开，分别按“MainSeq+SubSeq单调递增”的规则计算各自序列的丢包情况，即需要准备上海Order、上海Trade、上海XTS、深圳共4个序列。

-----------------------------------------------------------------------------------------------------------------------------------
/*
type=1.扫涨停：只使用逐笔成交的成交价判断距离涨停价有多少差距，如果满足则下单。 // p1-距涨停价x p2-买入的金额
type=2.触涨停：需要合成实时的订单簿，根据每次计算出的订单簿取卖一价是否涨停和卖一价档位的数量计算卖一档未成交的金额做比较，小于x万元则下单.  // p1-卖1金额小于x p2-买入的金额
type=3.排涨停：需要合成实时的订单簿，排涨停没有卖档，根据每次计算出的订单簿取买一档的数量计算买一档未成交的金额，如果大于x万元，则下单。 */   // p1-买1金额大于x p2-买入的金额