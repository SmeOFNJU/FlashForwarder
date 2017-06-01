var Index = function () {
	var updateInterval = 3000;//两次请求时间间隔	
	var total_received_bytes_thirty = {};//保存30s内入口流量
	var total_bytes_sent_thirty = {};//保存30s内出口流量
	var statusArray = {};//保存所有数据源5分钟的状态信息
	var visitors1_online = [];
	var visitors2_online = [];
	var visitors3_online = [];
	var visitors4_online = [];
	
	var dynamicArray = {};//保存所有数据源实时状态信息
	var dynamic_online = [];//
	
	var speedStatus = {};//保存所有入口速度 的最大值  最小值  实时值及时间
	var speed_online = [];
	
	var outputspeedStatus = {};//保存所出口速度 的最大值  最小值  实时值及时间
	var output_speed_online = [];
    return {
        //main function
        init: function () {
            Metronic.addResizeHandler(function () {
                jQuery('.vmaps').each(function () {
                    var map = jQuery(this);
                    map.width(map.parent().width());
                });
            });
        },
		initChartArray: function(hostName,index){
			if(""==index||null==index||undefined==index){
				index = 0;
			}
			visitors1_online = statusArray[hostName+index+'visitors1'];
			visitors2_online = statusArray[hostName+index+'visitors2'];
			visitors3_online = statusArray[hostName+index+'visitors3'];
			visitors4_online = statusArray[hostName+index+'visitors4'];
			dynamic_online = dynamicArray[hostName+index];
			speed_online = speedStatus[hostName+index];
			output_speed_online = outputspeedStatus[hostName+index];
		},
		
        initCharts: function (hostName) {
			//alert(hostName);
			//alert(urlConfig)
            if (!jQuery.plot) {
                return;
            }
	
            function showChartTooltip(x, y, xValue, yValue) {
                $('<div id="tooltip" class="chart-tooltip">' + yValue + '<\/div>').css({
                    position: 'absolute',
                    display: 'none',
                    top: y - 40,
                    left: x - 40,
                    border: '0px solid #ccc',
                    padding: '2px 6px',
                    'background-color': '#fff'
                }).appendTo("body").fadeIn(200);
            }
			
			
			
            var data = [];
            var totalPoints = 250;
			
			// random data generator for plot charts

            function getRandomData() {
                if (data.length > 0) data = data.slice(1);
                // do a random walk
                while (data.length < totalPoints) {
                    var prev = data.length > 0 ? data[data.length - 1] : 50;
                    var y = prev + Math.random() * 10 - 5;
                    if (y < 0) y = 0;
                    if (y > 100) y = 100;
                    data.push(y);
                }
                // zip the generated y values with the x values
                var res = [];
                for (var i = 0; i < data.length; ++i) {
                    res.push([i, data[i]]);
                }

                return res;
            }
			
			
			
			//四个图的绘图方法
			var creatChart1;
			var creatChart2;
			var creatChart3;
			var creatChart4;
			
			
			function chart1(){
				//入口速率
				if ($('#inputspeed_statistics').size() != 0) {

					$('#site_statistics_loading1').hide();
					$('#site_statistics_content1').show();
				
					var plot_statistics =null;
					
					creatChart1 = function(){
					
					plot_statistics = $.plot($("#inputspeed_statistics"),
						[{
							data: visitors1_online,
							lines: {
								fill: 0.18,
								lineWidth: 0.1,
								fillColor: {
									colors: [{
										opacity: 0.1
									}, {
										opacity: 1
									}]
								}
							},
							color: ['#34aa44']
						}, {
							data: visitors1_online,
							points: {
								show: false,
								fill: true,
								radius: 0.5,
								fillColor: "#34aa44",
								lineWidth: 3
							},
							color: '#34aa44',
							shadowSize: 0
						}, {
							data: visitors1_online,
							lines: {
								show: false,
								fill: false,
								lineWidth: 0.5
							},
							color: '#34aa44',
							shadowSize: 0
						}],

						{
							xaxis: {
								show:false,
								tickLength: 0,
								tickDecimals: 0,
								//mode: "categories",
								min: 0,
								max:100,
								font: {
									lineHeight: 14,
									style: "normal",
									variant: "small-caps",
									color: "#2e465d"
								}
							},
							yaxis: {
								ticks: 5,
								tickDecimals: 0,
								min: 0,
								tickColor: "#eee",
								font: {
									lineHeight: 14,
									style: "normal",
									variant: "small-caps",
									color: "#2e465d"
								}
							},
							grid: {
								hoverable: true,
								clickable: true,
								tickColor: "#eee",
								borderColor: "#eee",
								borderWidth: 1
							}
						});
						}
					var previousPoint = null;
					$("#inputspeed_statistics").bind("plothover", function (event, pos, item) {
						$("#x").text(pos.x.toFixed(2));
						$("#y").text(pos.y.toFixed(2));
						if (item) {
							if (previousPoint != item.dataIndex) {
								previousPoint = item.dataIndex;

								$("#tooltip").remove();
								var x = item.datapoint[0].toFixed(2),
									y = item.datapoint[1].toFixed(2);

								showChartTooltip(item.pageX, item.pageY, item.datapoint[0], item.datapoint[1] + ' KB/S');
							}
						} else {
							$("#tooltip").remove();
							previousPoint = null;
						}
					});
				}
			}
			
			function chart2(){
				//出口速率
				if ($('#outputspeed_statistics').size() != 0) {

					$('#site_statistics_loading2').hide();
					$('#site_statistics_content2').show();
					var plot_statistics =null;
					
					creatChart2 = function(){
					
					plot_statistics = $.plot($("#outputspeed_statistics"),
						[{
							data: visitors2_online,
							lines: {
								fill: 0.18,
								lineWidth: 0,
								fillColor: {
									colors: [{
										opacity: 0.1
									}, {
										opacity: 1
									}]
								}
							},
							color: ['#efc40e']
						}, {
							data: visitors2_online,
							points: {
								show: false,
								fill: true,
								radius: 4,
								fillColor: "#efc40e",
								lineWidth: 3
							},
							color: '#efc40e',
							shadowSize: 0
						}, {
							data: visitors2_online,
							lines: {
								show: true,
								fill: false,
								lineWidth: 0.5
							},
							fillColor: {
									colors: [{
										opacity: 0.1
									}, {
										opacity: 1
									}]
								},
							color: '#efc40e',
							shadowSize: 0
						}],

						{
							xaxis: {
								show:false,
								tickLength: 0,
								tickDecimals: 0,
								//mode: "categories",
								min: 0,
								max:100,
								font: {
									lineHeight: 14,
									style: "normal",
									variant: "small-caps",
									color: "#2e465d"
								}
							},
							yaxis: {
								ticks: 5,
								tickDecimals: 0,
								min: 0,
								tickColor: "#eee",
								font: {
									lineHeight: 14,
									style: "normal",
									variant: "small-caps",
									color: "#2e465d"
								}
							},
							grid: {
								hoverable: true,
								clickable: true,
								tickColor: "#eee",
								borderColor: "#eee",
								borderWidth: 1
							}
						});
						}
					var previousPoint = null;
					$("#outputspeed_statistics").bind("plothover", function (event, pos, item) {
						$("#x").text(pos.x.toFixed(2));
						$("#y").text(pos.y.toFixed(2));
						if (item) {
							if (previousPoint != item.dataIndex) {
								previousPoint = item.dataIndex;

								$("#tooltip").remove();
								var x = item.datapoint[0].toFixed(2),
									y = item.datapoint[1].toFixed(2);

								showChartTooltip(item.pageX, item.pageY, item.datapoint[0], item.datapoint[1] + ' KB/S');
							}
						} else {
							$("#tooltip").remove();
							previousPoint2 = null;
						}
					});					
				}
			}
			
			
			function chart3(){
				//连接数
				if ($('#session_count_statistics').size() != 0) {

					$('#site_statistics_loading3').hide();
					$('#site_statistics_content3').show();
			
					var plot_statistics =null;
					
					creatChart3 = function(){
					
					plot_statistics = $.plot($("#session_count_statistics"),
						[{
							data: visitors3_online,
							lines: {
								fill: 0.18,
								lineWidth: 0,
								fillColor: {
									colors: [{
										opacity: 0.1
									}, {
										opacity: 1
									}]
								}
							},
							color: ['#f79e9b']
						}, {
							data: visitors3_online,
							points: {
								show: false,
								fill: true,
								radius: 4,
								fillColor: "#f79e9b",
								lineWidth: 3
							},
							color: '#f79e9b',
							shadowSize: 0
						}, {
							data: visitors3_online,
							lines: {
								show: true,
								fill: false,
								lineWidth: 0.5
							},
							color: '#f79e9b',
							shadowSize: 0
						}],

						{
							xaxis: {
								show:false,
								tickLength: 0,
								tickDecimals: 0,
								//mode: "categories",
								min: 0,
								max:100,
								font: {
									lineHeight: 14,
									style: "normal",
									variant: "small-caps",
									color: "#2e465d"
								}
							},
							yaxis: {
								ticks: 5,
								tickDecimals: 0,
								min: 0,
								tickColor: "#eee",
								font: {
									lineHeight: 14,
									style: "normal",
									variant: "small-caps",
									color: "#2e465d"
								}
							},
							grid: {
								hoverable: true,
								clickable: true,
								tickColor: "#eee",
								borderColor: "#eee",
								borderWidth: 1
							}
						});
						}
					var previousPoint = null;
					$("#session_count_statistics").bind("plothover", function (event, pos, item) {
						$("#x").text(pos.x.toFixed(2));
						$("#y").text(pos.y.toFixed(2));
						if (item) {
							if (previousPoint != item.dataIndex) {
								previousPoint = item.dataIndex;

								$("#tooltip").remove();
								var x = item.datapoint[0].toFixed(2),
									y = item.datapoint[1].toFixed(2);

								showChartTooltip(item.pageX, item.pageY, item.datapoint[0], item.datapoint[1] + ' 个');
							}
						} else {
							$("#tooltip").remove();
							previousPoint2 = null;
						}
					});
				}
			}
			function chart4(){
				//压缩比
				if ($('#compress_rate_statistics').size() != 0) {

					$('#site_statistics_loading4').hide();
					$('#site_statistics_content4').show();
				
			
					var plot_statistics =null;
					
					creatChart4 = function(){
					
					plot_statistics = $.plot($("#compress_rate_statistics"),
						[{
							data: visitors4_online,
							lines: {
								fill: 0.18,
								lineWidth: 0,
								fillColor: {
									colors: [{
										opacity: 0.1
									}, {
										opacity: 1
									}]
								}
							},
							color: ['#649ddf']
						}, {
							data: visitors4_online,
							points: {
								show: false,
								fill: true,
								radius: 4,
								fillColor: "#649ddf",
								lineWidth: 3
							},
							color: '#649ddf',
							shadowSize: 0
						}, {
							data: visitors4_online,
							lines: {
								show: true,
								fill: false,
								lineWidth: 0.5
							},
							color: '#649ddf',
							shadowSize: 0
						}],

						{
							xaxis: {
								show:false,
								tickLength: 0,
								tickDecimals: 0,
								//mode: "categories",
								min: 0,
								max:100,
								font: {
									lineHeight: 14,
									style: "normal",
									variant: "small-caps",
									color: "#2e465d"
								}
							},
							yaxis: {
								ticks: 5,
								tickDecimals: 0,
								min: 0,
								tickColor: "#eee",
								font: {
									lineHeight: 14,
									style: "normal",
									variant: "small-caps",
									color: "#2e465d"
								}
							},
							grid: {
								hoverable: true,
								clickable: true,
								tickColor: "#eee",
								borderColor: "#eee",
								borderWidth: 1
							}
						});
						}
					var previousPoint = null;
					$("#compress_rate_statistics").bind("plothover", function (event, pos, item) {
						$("#x").text(pos.x.toFixed(2));
						$("#y").text(pos.y.toFixed(2));
						if (item) {
							if (previousPoint != item.dataIndex) {
								previousPoint = item.dataIndex;

								$("#tooltip").remove();
								var x = item.datapoint[0].toFixed(2),
									y = item.datapoint[1].toFixed(2);

								showChartTooltip(item.pageX, item.pageY, item.datapoint[0], item.datapoint[1] + ' 倍');
							}
						} else {
							$("#tooltip").remove();
							previousPoint2 = null;
						}
					});
				}
			}
			//Dynamic Chart
			//var plot;
            function chart5() {
                if ($('#chart_4').size() != 1) {
                    return;
                }
                //server load
                var options = {
                    series: {
                        shadowSize: 1
                    },
                    lines: {
                        show: true,
                        lineWidth: 0.5,
                        fill: true,
                        fillColor: {
                            colors: [{
                                opacity: 0.1
                            }, {
                                opacity: 1
                            }]
                        }
                    },
                    yaxis: {
                        min: 0,
                        max: 2000000,
                        tickColor: "#eee",
                        tickFormatter: function(v) {
                            return v;
                        }
                    },
                    xaxis: {
                        show: true,
						min: 0,
                        max: 100,
                    },
                    colors: ["#6ef146"],
                    grid: {
                        tickColor: "#eee",
                        borderWidth: 0,
                    }
                };

                var updateInterval = 30;
				//if (data.length > 0) data = data.slice(1);
                // do a random walk
               
				var res = [];
				res.push([1,30]);
				res.push([2,50]);
                plot = $.plot($("#chart_4"), [getRandomData()], options);
				//plot.setData([[2,10]]);
                plot.draw();
                function update() {
                    plot.setData([getRandomData()]);
                    plot.draw();
                    setTimeout(update, updateInterval);
                }
                update();
            }

			//graph
			chart1();
			chart2();	
			chart3();
			chart4();
			//chart5();
			
			//当图片上画满100个点时，每次刷新X轴为0-100
			function XControl(visitor){  
				for (var i = 0; i < visitor.length; i++) {
					//visitor[i][0] = visitor[i][0] - 1;
					visitor[i][0] = i;
				}
			}
			
			var i = 0;
			//var speedIndex = 0;
			function update() {
				//if(Index.updateFnCallId!=-1){
				//	clearTimeout(Index.updateFnCallId);
				//}
				$.ajax({
					url: urlStatus,
					async:true,
					type:"post", 
					success:function(data){
						/*start 保存数据*/
						var statusDetails = data;//所有主机状态信息
						var hostNames = new Array();//所有主机名
						for(var keyName in statusDetails){
							hostNames.push(keyName);
						}
						
						
						for(var hostIndex=0;hostIndex<hostNames.length;hostIndex++){
							var host_Name = hostNames[hostIndex];
							var mMax;
							var oneHostStatusDetail;
							if(statusDetails[host_Name]!=null&&""!=statusDetails[host_Name]){
								if(statusDetails[host_Name].length==undefined){
									mMax = 1;
								}else{
									mMax = statusDetails[host_Name].length;
								}
								for(var m=0;m<mMax;m++){
									if(mMax==1){
										oneHostStatusDetail = statusDetails[host_Name];//单个源
									}else{
										oneHostStatusDetail = statusDetails[host_Name][m];//多个源
									}
									var total_received_bytes;//入口流量
									var total_bytes_sent;//出口流量
									var session_count;//连接数
									//已读取字节
									total_received_bytes = Math.round(oneHostStatusDetail.total_received_bytes/1024);
									total_bytes_sent = Math.round(oneHostStatusDetail.total_bytes_sent/1024);
									session_count = oneHostStatusDetail.session_count;
									
									//visitors1.push([i,total_received_bytes]);
									//visitors2.push([i,total_bytes_sent]);
									//visitors3.push([i,session_count]);
									if(statusArray[host_Name+m+'visitors1']==undefined){
										statusArray[host_Name+m+'visitors1'] = [];
									}
									if(statusArray[host_Name+m+'visitors2']==undefined){
										statusArray[host_Name+m+'visitors2'] = [];
									}
									if(statusArray[host_Name+m+'visitors3']==undefined){
										statusArray[host_Name+m+'visitors3'] = [];
									}
									if(statusArray[host_Name+m+'visitors4']==undefined){
										statusArray[host_Name+m+'visitors4'] = [];
									}
									if(statusArray[host_Name+m+'timeandspeed']==undefined){
										statusArray[host_Name+m+'timeandspeed'] = [];
									}
									if(statusArray[host_Name+m+'timeandoutputspeed']==undefined){
										statusArray[host_Name+m+'timeandoutputspeed'] = [];
									}
									statusArray[host_Name+m+'visitors3'].push([i,session_count]);//连接数
									var compression_ratio;
									if(oneHostStatusDetail.total_received_bytes!="0"){
										compression_ratio = oneHostStatusDetail.total_decoded_bytes/oneHostStatusDetail.total_received_bytes;
									}else{
										compression_ratio = "0";
									}
									statusArray[host_Name+m+'visitors4'].push([i,compression_ratio]);//压缩比

									var speed;//入口速率
									//计算速度,total_received_bytes_thirty数组中最多11个元素，即30s平均速度
									if(total_received_bytes_thirty[host_Name+m+'speed']==undefined){
										total_received_bytes_thirty[host_Name+m+'speed'] = [];
									}
									//alert(oneHostStatusDetail.last_msg_timestamp)
									total_received_bytes_thirty[host_Name+m+'speed'].push([oneHostStatusDetail.last_msg_timestamp,oneHostStatusDetail.total_received_bytes]);
									if(total_received_bytes_thirty[host_Name+m+'speed'].length>=2){
										var middleArray = total_received_bytes_thirty[host_Name+m+'speed'];
										if((middleArray[middleArray.length-1][0]-middleArray[0][0])!=0){
											speed = (middleArray[middleArray.length-1][1]-middleArray[0][1])/(middleArray[middleArray.length-1][0]-middleArray[0][0])/1.024; 
										}else{
											speed = 0;
										}
										
										speed = speed.toFixed(2);
										statusArray[host_Name+m+'visitors1'].push([i,speed]);
										statusArray[host_Name+m+'timeandspeed'].push([oneHostStatusDetail.last_msg_timestamp,speed]);
									}
									if(total_received_bytes_thirty[host_Name+m+'speed'].length==11){
										total_received_bytes_thirty[host_Name+m+'speed'].shift();
									}
									
									var outputspeed;//出口速率
									//计算速度,total_bytes_sent_thirty数组中最多11个元素，即30s平均速度
									if(total_bytes_sent_thirty[host_Name+m+'speed']==undefined){
										total_bytes_sent_thirty[host_Name+m+'speed'] = [];
									}
									//alert(oneHostStatusDetail.last_msg_timestamp)
									total_bytes_sent_thirty[host_Name+m+'speed'].push([oneHostStatusDetail.last_msg_timestamp,oneHostStatusDetail.total_bytes_sent]);
									if(total_bytes_sent_thirty[host_Name+m+'speed'].length>=2){
										var middleArray = total_bytes_sent_thirty[host_Name+m+'speed'];
										if((middleArray[middleArray.length-1][0]-middleArray[0][0])!=0){
											outputspeed = (middleArray[middleArray.length-1][1]-middleArray[0][1])/(middleArray[middleArray.length-1][0]-middleArray[0][0])/1.024; 
										}else{
											outputspeed = 0;
										}
										
										outputspeed = outputspeed.toFixed(2);
										statusArray[host_Name+m+'visitors2'].push([i,outputspeed]);
										statusArray[host_Name+m+'timeandoutputspeed'].push([oneHostStatusDetail.last_msg_timestamp,outputspeed]);
									}
									if(total_bytes_sent_thirty[host_Name+m+'speed'].length==11){
										total_bytes_sent_thirty[host_Name+m+'speed'].shift();
									}
									//5分钟3S一个点，总共101个点，102个元素的时候先改变下x轴数据，再把第一个删除
									if(statusArray[host_Name+m+'visitors1'].length>101){
										statusArray[host_Name+m+'visitors1'].shift();
										XControl(statusArray[host_Name+m+'visitors1']);
										statusArray[host_Name+m+'timeandspeed'].shift();
									}
									if(statusArray[host_Name+m+'visitors2'].length>101){
										statusArray[host_Name+m+'visitors2'].shift();
										XControl(statusArray[host_Name+m+'visitors2']);
										statusArray[host_Name+m+'timeandoutputspeed'].shift();
									}
									if(statusArray[host_Name+m+'visitors3'].length>101){
										statusArray[host_Name+m+'visitors3'].shift();
										XControl(statusArray[host_Name+m+'visitors3']);
									}
									if(statusArray[host_Name+m+'visitors4'].length>101){
										statusArray[host_Name+m+'visitors4'].shift();
										XControl(statusArray[host_Name+m+'visitors4']);
									}
									/*start 存储动态信息 dynamicArray[host_Name+m] 0最近消息时间 1连接建立时间 2连接状态*/
									if(dynamicArray[host_Name+m]==undefined){
										dynamicArray[host_Name+m] = [];
										dynamicArray[host_Name+m].push(oneHostStatusDetail.last_msg_timestamp);
										dynamicArray[host_Name+m].push(oneHostStatusDetail.last_session_timestamp);
										dynamicArray[host_Name+m].push(oneHostStatusDetail.connect_status);
										dynamicArray[host_Name+m].push(oneHostStatusDetail.total_bytes_sent);
										dynamicArray[host_Name+m].push(oneHostStatusDetail.total_decoded_bytes);
										dynamicArray[host_Name+m].push(oneHostStatusDetail.total_received_bytes);
										dynamicArray[host_Name+m].push(oneHostStatusDetail.active_connection);
										dynamicArray[host_Name+m].push(oneHostStatusDetail.total_missed_packets);
									}else{
										dynamicArray[host_Name+m][0] = oneHostStatusDetail.last_msg_timestamp;
										dynamicArray[host_Name+m][1] = oneHostStatusDetail.last_session_timestamp;
										dynamicArray[host_Name+m][2] = oneHostStatusDetail.connect_status;
										dynamicArray[host_Name+m][3] = oneHostStatusDetail.total_bytes_sent;
										dynamicArray[host_Name+m][4] = oneHostStatusDetail.total_decoded_bytes;
										dynamicArray[host_Name+m][5] = oneHostStatusDetail.total_received_bytes;
										dynamicArray[host_Name+m][6] = oneHostStatusDetail.active_connection;
										dynamicArray[host_Name+m][7] = oneHostStatusDetail.total_missed_packets;
									}
									/*end 存储动态信息 dynamicArray[host_Name+m] 0最近消息时间 1连接建立时间 2连接状态*/
									
									/*start 存储速度和时间 speedStatus[host_Name+m] 0实时值 1最小值 2最大值*/
									if(speedStatus[host_Name+m]==undefined){
										speedStatus[host_Name+m] = [];
									}
									
									if(statusArray[host_Name+m+'timeandspeed'].length>=1){
										//得到5分钟内的最大最小速度和出现时间
										var maxTime = statusArray[host_Name+m+'timeandspeed'][0][0];
										var minTime = statusArray[host_Name+m+'timeandspeed'][0][0];
										var max_speed = statusArray[host_Name+m+'timeandspeed'][0][1];
										var min_speed = statusArray[host_Name+m+'timeandspeed'][0][1];
										for(var n =0;n<statusArray[host_Name+m+'timeandspeed'].length;n++){
											if(parseFloat(max_speed) < parseFloat(statusArray[host_Name+m+'timeandspeed'][n][1])){
												max_speed = statusArray[host_Name+m+'timeandspeed'][n][1];
												maxTime = statusArray[host_Name+m+'timeandspeed'][n][0];
											}
											if(parseFloat(min_speed) > parseFloat(statusArray[host_Name+m+'timeandspeed'][n][1])){
												min_speed = statusArray[host_Name+m+'timeandspeed'][n][1];
												minTime = statusArray[host_Name+m+'timeandspeed'][n][0];
											}										
										}
										//存储速度和时间
										if(speedStatus[host_Name+m].length==0){
											//speedStatus[host_Name+m] 0实时值 1最小值 2最大值
											speedStatus[host_Name+m].push([oneHostStatusDetail.last_msg_timestamp,speed]);//实时值
											speedStatus[host_Name+m].push([minTime,min_speed]);
											speedStatus[host_Name+m].push([maxTime,max_speed]);
										}else{
											speedStatus[host_Name+m][0] = [oneHostStatusDetail.last_msg_timestamp,speed];//实时值
											//最小值
											if(parseFloat(speedStatus[host_Name+m][1][1])>parseFloat(min_speed)){
												speedStatus[host_Name+m][1] = [minTime,min_speed];
											}
											//最大值
											if(parseFloat(speedStatus[host_Name+m][2][1])<parseFloat(max_speed)){
												speedStatus[host_Name+m][2] = [maxTime,max_speed];
											}
										}
									}
									/*end 存储速度和时间 speedStatus[host_Name+m] 0实时值 1最小值 2最大值*/
									
									/*start 存储出口速度和时间 outputspeedStatus[host_Name+m] 0实时值 1最小值 2最大值*/
									if(outputspeedStatus[host_Name+m]==undefined){
										outputspeedStatus[host_Name+m] = [];
									}
									
									if(statusArray[host_Name+m+'timeandoutputspeed'].length>=1){
										//得到5分钟内的最大最小速度和出现时间
										var maxTime = statusArray[host_Name+m+'timeandoutputspeed'][0][0];
										var minTime = statusArray[host_Name+m+'timeandoutputspeed'][0][0];
										var max_speed = statusArray[host_Name+m+'timeandoutputspeed'][0][1];
										var min_speed = statusArray[host_Name+m+'timeandoutputspeed'][0][1];
										for(var n =0;n<statusArray[host_Name+m+'timeandoutputspeed'].length;n++){
											if(parseFloat(max_speed) < parseFloat(statusArray[host_Name+m+'timeandoutputspeed'][n][1])){
												max_speed = statusArray[host_Name+m+'timeandoutputspeed'][n][1];
												maxTime = statusArray[host_Name+m+'timeandoutputspeed'][n][0];
											}
											if(parseFloat(min_speed) > parseFloat(statusArray[host_Name+m+'timeandoutputspeed'][n][1])){
												min_speed = statusArray[host_Name+m+'timeandoutputspeed'][n][1];
												minTime = statusArray[host_Name+m+'timeandoutputspeed'][n][0];
											}										
										}
										//存储速度和时间
										if(outputspeedStatus[host_Name+m].length==0){
											//outputspeedStatus[host_Name+m] 0实时值 1最小值 2最大值
											outputspeedStatus[host_Name+m].push([oneHostStatusDetail.last_msg_timestamp,outputspeed]);//实时值
											outputspeedStatus[host_Name+m].push([minTime,min_speed]);
											outputspeedStatus[host_Name+m].push([maxTime,max_speed]);
										}else{
											outputspeedStatus[host_Name+m][0] = [oneHostStatusDetail.last_msg_timestamp,outputspeed];//实时值
											//最小值
											if(parseFloat(outputspeedStatus[host_Name+m][1][1])>parseFloat(min_speed)){
												outputspeedStatus[host_Name+m][1] = [minTime,min_speed];
											}
											//最大值
											if(parseFloat(outputspeedStatus[host_Name+m][2][1])<parseFloat(max_speed)){
												outputspeedStatus[host_Name+m][2] = [maxTime,max_speed];
											}
										}
									}
									/*end 存储出口速度和时间 outputspeedStatus[host_Name+m] 0实时值 1最小值 2最大值*/
										
								}
								
							}
						}
						if(i<101){
							i++;
						}
						/*end 保存数据*/
						
						//设置默认数据源，默认动态时间数组，hostName对应的第一个源
						if(visitors1_online==""||visitors1_online==null){
							visitors1_online = statusArray[hostName+0+'visitors1'];
							visitors2_online = statusArray[hostName+0+'visitors2'];
							visitors3_online = statusArray[hostName+0+'visitors3'];
							visitors4_online = statusArray[hostName+0+'visitors4'];
							dynamic_online = dynamicArray[hostName+0];
							speed_online = speedStatus[hostName+0];
							output_speed_online = outputspeedStatus[hostName+0];
							
						}
						
						creatChart1();
						creatChart2();
						creatChart3();
						creatChart4();
						
						
						/*start 实时刷新状态值*/
						
						$("#instantaneous_session_count").html(visitors3_online[visitors3_online.length-1][1]+"个");//连接数
						
						
						//入口速度实时值
						if(speed_online!=undefined&&speed_online.length>0){
							//速度0实时值 1最小值 2最大值
							$("#instantaneous_speed_time").html(getLocalTime(speed_online[0][0]));
							$("#instantaneous_min_speed_time").html(getLocalTime(speed_online[1][0]));
							$("#instantaneous_max_speed_time").html(getLocalTime(speed_online[2][0]));
							
							$("#instantaneous_speed").html(speed_online[0][1]+"KB/S");
							$("#instantaneous_min_speed").html(speed_online[1][1]+"KB/S");
							$("#instantaneous_max_speed").html(speed_online[2][1]+"KB/S");				
						}
						
						//出口速度实时值
						if(output_speed_online!=undefined&&output_speed_online.length>0){
							//速度0实时值 1最小值 2最大值
							//$("#instantaneous_outputspeed_time").html(getLocalTime(output_speed_online[0][0]));
							$("#instantaneous_min_outputspeed_time").html(getLocalTime(output_speed_online[1][0]));
							$("#instantaneous_max_outputspeed_time").html(getLocalTime(output_speed_online[2][0]));
							
							$("#instantaneous_outputspeed").html(output_speed_online[0][1]+"KB/S");
							$("#instantaneous_min_outputspeed").html(output_speed_online[1][1]+"KB/S");
							$("#instantaneous_max_outputspeed").html(output_speed_online[2][1]+"KB/S");				
						}
						
						/*
						*status实时值
						*0 last_msg_timestamp
						*1 last_session_timestamp 最近连接时间
						*2 connect_status 连接状态
						*3 total_bytes_sent 已发送字节数
						*4 total_decoded_bytes 已解压字节数
						*5 total_received_msgs 已接收字节数
						*6 active_connection  用于TCP更新数据源，多个路径，分割
						*/
						if(dynamic_online!=""&&dynamic_online!=null){
							$("#last_session_timestamp").html(getLocalTime(dynamic_online[1]));//刷新连接建立时间
							$("#connect_status").html(returnStatusHtml(dynamic_online[2]));//实时更新连接状态
							$("#total_bytes_sent").html(dynamic_online[3]+"B");//已发送字节数
							$("#total_decoded_bytes").html(dynamic_online[4]+"B");//已解压字节数
							$("#total_received_msgs").html(dynamic_online[5]+"B");//已接收字节数
							$("#total_missed_packets").html(dynamic_online[7]+"");//丢包数
							if(dynamic_online[6]>=1){
								var oneHostConfigData = dataConfigDetails[hostName];//当前主机配置信息
								var source = oneHostConfigData.source;//sourceArray一个主机有多个source,用，分隔开
								var sourceString = source.toString();//对象转字符串
								var sourceArray = new Array();
								sourceArray = sourceString.split("|");
								if(sourceArray.length==1){
									sourceArray = sourceString.split(",");
								}
								//alert(formatURL(sourceArray[dynamic_online[6]]))
								$("#source").html(formatURL(sourceArray[dynamic_online[6]]));
							}
						}
						
						
						/*end 实时刷新状态值*/
						
						//Index.updateFnCallId = setTimeout(update, updateInterval);
						setTimeout(update, updateInterval);
					}
				});
		 
			}
			update();
			
		}
		//updateFnCallId:-1
			

		

    };

}();

/***  获取url地址参数   ****/
jQuery.extend({
	getPara: function (name, url) {
        var reg = new RegExp("(^|&)" + name + "=([^&]*)(&|$)");
        if (url) {
            if (url.indexOf("?") != -1)
                url = url.split("?")[1];
        } else if (window.location.search.length > 1) {
            url = decodeURI(window.location.search.substr(1));
        } else {
            return '';
        }
        var r = url.match(reg);
        if (r != null) return unescape(r[2]); return '';
    }
});