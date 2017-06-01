//声明一个应用
var myApp = angular.module("myApp", []);
//初始化
window.onload = function(){		    	
   //Metronic.init(); // init metronic核心组件
    
}

//创建一个控制器
myApp.controller("myController", ["$scope","$http","$interval",function ($scope,$http,$interval) {	
	//列表分页
	$scope.paginationConf = {
        list: [],//分页保存的数据
        currentPage: 1,//默认当前第一页
        totalItems: 0,
        itemsPerPage: 50,
        pagesLength: 50,
        perPageOptions: [10, 15, 30, 50, 100, 200, 300],
        orderBy: '',
        query: '',
        rememberPerPage: 'perPageItems',
        noDataTip: '暂无数据！',
        onChange: function () {
        }
    };
	
	
	//请求http服务获取列表ip和端口号
	var jsondata =[];
	var Nodeport=$.getPara("nodeport");
	if(Nodeport==""){
		Nodeport=8080;
	}
	var ResultData=[];
	var recordPk =[];

	
	//定义报警器式样
	$scope.errorCounter = 0; //record how many hosts are in error status.
	$scope.alarmShow = false; //switch between starting and stopping status.
	$scope.alarm = document.getElementById("alarm");
	$scope.alarmSwitch = document.getElementById("alarmSwitch");
	$("#alarm").css({"float":"right","padding-left":"1%","padding-right":"3%"});
	$("#alarmSwitch").css({"float":"right"});
	$("#alarm").hide();
	$("#alarmSwitch").click(function(){
		if($scope.alarmShow){
			$scope.alarm.pause();	
			$scope.alarm.currentTime = 0;
			$("#alarm").hide();
			$scope.alarmShow = false;
			$scope.alarmSwitch.innerHTML = "开启报警声";
		}else{
			$("#alarm").show();
			$scope.alarmShow = true;
			$scope.alarmSwitch.innerHTML = "关闭报警声";	
		}
	});
	
    $http.jsonp("/list?callback=JSON_CALLBACK").success(function (nodedata, header, config,status) {
    	
        jsondata = nodedata;
		//#region 初始化
		var readdata;
		var RequestHttp = function(){
		if($scope.errorCounter != 0 &&
			$scope.alarmShow){
			$scope.alarm.play();
		}else{
			$scope.alarm.currentTime = 0;
			$scope.alarm.pause();
		}
		    readdata = jsondata;
		    //是否存在
			if(readdata){
			   var data =readdata.data;
			   if(data && data.length > 0){
			   	 $.each(data,function(index,items){
			   	 	var branchName = items.id; // ID编号
			   	 	var _ip = items.ip;  // IP地址
			   	 	var _portip = items.port; // 端口号
			   	 	var comobj ={};
			   	 	comobj.branchName = branchName;
			   	 	comobj.ip = _ip;
			   	 	comobj.port = _portip;
			   	 	comobj.pk = ""+_ip+"_"+_portip+"";
			   	 	(function getPortNodeInfo($http,_ip,_portip){
			   	      StatusHttpJsonp($scope,$http,branchName,_ip,_portip,comobj,ResultData,recordPk);
					})($http,_ip,_portip);
			   	 });
			   }
			}
		}
		//#endregion 初始化
		RequestHttp(); 
		  
		//自动刷新
		var mtime = setInterval(function(){
	      RequestHttp();
	    },5000);
        
	}).error(function (data, status, headers,config) {
    }); 

	//绑定数据
	$scope.list =ResultData;
	$scope.paginationConf.list = ResultData;
    $scope.paginationConf.totalItems = ResultData.length;
    $("#monit_list").show();
	//$scope.$apply();
	$scope.$watch('savejosn',function(){
		//每重新请求数据 重新加载一遍
		$scope.list =ResultData;
		$scope.paginationConf.list = ResultData;
        $scope.paginationConf.totalItems = ResultData.length;
		//$scope.$apply();
	});
	
	
	
	
}]);

/**  Status Http请求  **/
function StatusHttpJsonp($scope,$http,branchName,_ip,_portip,comobj,ResultData,recordPk){
   $http.jsonp("http://"+_ip+":"+_portip+"/status?callback=JSON_CALLBACK").success(function (data, status, headers,config) {
   	    var pk =""+_ip+"_"+_portip+"";
   	    //已经存在的IP和端口号
		if($.inArray(pk,recordPk)>-1){
			//判断IP端口号之前 请求是异常 删除异常后添加该ip端口的市场数据
			if(isError(pk,recordPk,ResultData)){
				//不存在的IP和端口号
			    ComStatusData($scope,$http,branchName,_ip,_portip,comobj,ResultData,recordPk,data);
				$scope.errorCounter--;  //the error host had recovered.
			}else{
				//ip端口号相同且也不是异常状态。更新ip端口号市场、状态、已接收消息数、最近消息时间、已发送字节字段数据
				ComSameStatusData($scope,$http,branchName,_ip,_portip,comobj,ResultData,recordPk,data);
			}
		}else{
		    //不存在的IP和端口号
			ComStatusData($scope,$http,branchName,_ip,_portip,comobj,ResultData,recordPk,data);
		}
   		
		
   }).error(function (data, status, headers,config) {
   	    var pk =""+_ip+"_"+_portip+"";
   	    //判断IP和端口号已经绑定数据了 <0 否 反则 是
		if($.inArray(pk,recordPk)<0){
			//（针对 status请求失败的情况）
			//记录添加的ip和端口号并且防止定时请求时 重复添加
			//添加的ID和端口号，并记录下来
			$scope.errorCounter++;
			recordPk.push(pk);
			comobj.branchName =branchName;
	    	comobj.bazaar="异常";
	    	comobj.connect_status ="异常";
	    	comobj.total_received_bytes ="--";
	    	comobj.last_msg_timestamp ="--";
	    	comobj.total_bytes_sent ="--";
	    	comobj.highlight="higok";
	    	comobj.islink = "linkno";
	    	comobj.moreBazaar=pk+"_异常"; 
	    	ResultData.push(comobj);
	    	//触发更新
		    $scope.savejosn =comobj;
		}else{
			//（针对 status请求失败的情况并且之前是请求成功的，后面关了服务的情况 做处理）
			//删除列表已经绑定数据
			DeleteGrid(pk,ResultData,recordPk);
		}
   }); 
}

// Status 组合数据定列表
function ComStatusData($scope,$http,branchName,_ip,_portip,comobj,ResultData,recordPk,data){
	//得到status状态配置的市场、连接状态、接收消息数、最近消息时间、已发送字节
	for(var keyname in data){
		var pk =""+_ip+"_"+_portip+"";
		var reobj = data[""+keyname+""];
		//判断是否是市场
		if(reobj.is_market){
			reobj.bazaar=keyname; //市场名称
    		reobj.branchName = branchName; //列表ID编号
	   	 	reobj.ip = _ip;//Ip地址
	   	 	reobj.port =_portip; //端口号
	   	 	reobj.pk =pk; //PK
	   	 	reobj.moreBazaar=""+_ip+"_"+_portip+"_"+keyname+""; //唯一主键 IP_端口号_市场
	   	 	reobj.highlight ="higno";//不显示高亮
	   	 	comobj.islink = "linkok"; //显示操作查看按钮
	   	 	ResultData.push(reobj);
	   	 	//记录已经请求成功的ID和端口号
			if($.inArray(pk,recordPk)<0){
				recordPk.push(pk);//添加PK
			}
	   	 	//触发更新
	        $scope.savejosn =reobj;
		}
		
	}
	
}

// Status 定时请求 重新覆盖数据
function ComSameStatusData($scope,$http,branchName,_ip,_portip,comobj,ResultData,recordPk,data){
	for(var keyname in data){
		var pk =""+_ip+"_"+_portip+""; //PK
		var bazaar =keyname; //市场名称
		var moreBazaar = pk+"_"+bazaar;//唯一主键 IP_端口号_市场
		for(var j= 0; j<ResultData.length;j++){
			var moreBazaar2 =ResultData[j].moreBazaar;
			//主键等于存在的主键的  替换数据
			if(moreBazaar2==moreBazaar){
				var reobj = data[""+keyname+""];
				reobj.bazaar=bazaar; //市场名称
	    		reobj.branchName = branchName; //列表ID编号
		   	 	reobj.ip = _ip;//Ip地址
		   	 	reobj.port =_portip; //端口号
		   	 	reobj.pk =pk; //PK
		   	 	reobj.moreBazaar=moreBazaar; //唯一主键 IP_端口号_市场
		   	 	reobj.highlight ="higno";//不显示高亮
		   	 	comobj.islink = "linkok"; //显示操作查看按钮
				ResultData[j]=reobj;
				//触发更新
	            $scope.savejosn =reobj;
			}
		}
		
	}
}

/** IP端口号Http重新请求成功 删除原来的异常  **/
function isError(pk,recordPk,ResultData){
	var _false=false;
	if($.inArray(pk,recordPk)>-1){
		for(var k=0;k<ResultData.length;k++){
			if(ResultData[k].pk==pk){
				if(ResultData[k].bazaar=="异常"){
					ResultData.splice(k,1);
					_false = true;
				}
			}
		}
	}
	return _false;
}

//Status 请求失败，并且之前是请求成功。删除之前的请求成功数据
function DeleteGrid(pk,ResultData,recordPk){
	//记录是否有之前请求的数据被删除掉
	var _false=false;
	for(var k=0;k<ResultData.length;k++){
		if(ResultData[k].pk==pk){
			//改IP和端口号已经请求成功的数据删除掉（针对已经开服了服务请求正常，后面掉 了服务的情况做处理）
			if(ResultData[k].highlight =="higno"){
				ResultData.splice(k,1);
				k--;
				_false =true;
			}
		}
	}
	//只有删除过之前全部的正常数据之后，现在在删除对应的PK 定时请求好重新添加 该Ip端口号异常记录这一条数据
	if(_false){
		recordPk.splice($.inArray(pk,recordPk),1); 
	}
}


//连接状态转换
myApp.filter('fminute', function () {
    return function (datenum) {
        var time;
        if (datenum=="0" ||datenum==0) {
            time = '初始';
        }
        else if(datenum=="1" ||datenum==1){
        	time = '正在连接';
        }
        else if(datenum=="2" ||datenum==2){
        	time = '正在登录';
        }
        else if(datenum=="3" ||datenum==3){
        	time = '正常';
        }
        else if(datenum=="4" ||datenum==4){
        	time = '断开';
        }
        else if(datenum=="99" ||datenum==99){
        	time = '异常';
        }
        else{
        	time =datenum;
        }
        return time;
    };
});

myApp.filter('offset', function () {
    return function (input, start) {
        start = parseInt(start, 10);
        if (input && input.length > 0) {
            return input.slice(start);
        }
    };
});

//列表分页

/****************************** 分页   ******************************/
myApp.directive('pageLabel', ['$filter', function () {
    return {
        restrict: 'EA',
        template: '<div class="row">' +
                      '<div class="col-md-5 col-sm-12">' +
                          '<div class="dataTables_info" id="paging_info" role="status" aria-live="polite">' +
                              '每页显示<select class="re-form-group-page-select" ng-model="conf.itemsPerPage" ng-options="option for option in conf.perPageOptions " ng-change="changeItemsPerPage()"></select>条,总记录{{ conf.totalItems }}条,共{{conf.numberOfPages}}页' +
                          '</div>' +
                      '</div>' +
                     '<div class="col-md-7 col-sm-12">' +
                     '<div class="dataTables_paginate paging_simple_numbers" id="paging_paginate"  style="float:right">' +
                        '<ul class="pagination re-pagination-lg" style="float:left">' +
                            '<li class="paginate_button previous" id="paging_previous" ng-class="{disabled: conf.currentPage == 1}" ng-click="prevPage()">' +
                                '<a href="javascript:void(0)">上一页</a>' +
                            '</li>' +
                            '<li class="paginate_button" ng-repeat="item in pageList track by $index" ng-class="{active: item == conf.currentPage, separate: item == \'...\'}"  ng-click="changeCurrentPage(item)">' +
                              '<a href="javascript:void(0)">{{ item }}</a>' +
                            '</li>' +
                            ' <li class="paginate_button next" id="paging_next" ng-class="{disabled: conf.currentPage == conf.numberOfPages}" ng-click="nextPage()">' +
                               '<a href="javascript:void(0)"   >下一页</a>' +
                             ' </li>' +
                     ' </ul>' +
                     '<div style="float:left;margin: 2px 10px 0px 10px;">' +
                        '到第<input class="zc-page-num" type="text" ng-model="jumpPageNum"  ng-keyup="jumpToPage($event)"/>页' +
                     '</div>' +
                    '</div>' +
                   '</div>' +
              '</div>',
        replace: true,
        scope: {
            conf: '='
        },
        link: function (scope, element, attrs) {
            // 变更当前页
            scope.changeCurrentPage = function (item) {
                if (item == '...') {
                    return;
                } else {
                    scope.conf.currentPage = item;
                }
            };
            // 定义分页的长度必须为奇数 (default:9)
            scope.conf.pagesLength = parseInt(scope.conf.pagesLength) ? parseInt(scope.conf.pagesLength) : 9;
            if (scope.conf.pagesLength % 2 === 0) {
                // 如果不是奇数的时候处理一下
                scope.conf.pagesLength = scope.conf.pagesLength - 1;
            }
            // conf.erPageOptions 是否默认有值
            if (!scope.conf.perPageOptions) {
                scope.conf.perPageOptions = [10, 15, 30, 50, 100, 200];
            }
            /** pageList数组**/
            function getPagination() {
                // conf.currentPage 当前页是否有默认值
                scope.conf.currentPage = parseInt(scope.conf.currentPage) ? parseInt(scope.conf.currentPage) : 1;

                //是否有默认每页条数
                scope.conf.itemsPerPage = parseInt(scope.conf.itemsPerPage) ? parseInt(scope.conf.itemsPerPage) : 15;

                //获取总页数numberOfPages
                scope.conf.numberOfPages = Math.ceil(scope.conf.totalItems / scope.conf.itemsPerPage);

                //  当前页数是否大于1
                if (scope.conf.currentPage < 1) {
                    scope.conf.currentPage = 1;
                }
                // judge currentPage > scope.numberOfPages 当前页数是否大于总页数
                if (scope.conf.currentPage > scope.conf.numberOfPages) {
                    scope.conf.currentPage = scope.conf.numberOfPages;
                }

                // jumpPageNum
                scope.jumpPageNum = scope.conf.currentPage;

                // 如果itemsPerPage(默认每页条数)在不在perPageOptions（下拉框每页显示多少条）数组中，就把itemsPerPage加入这个数组中
                var perPageOptionsLength = scope.conf.perPageOptions.length;
                // 定义状态
                var perPageOptionsStatus;
                for (var i = 0; i < perPageOptionsLength; i++) {
                    if (scope.conf.perPageOptions[i] == scope.conf.itemsPerPage) {
                        perPageOptionsStatus = true;
                    }
                }
                // 如果itemsPerPage在不在perPageOptions数组中，就把itemsPerPage加入这个数组中
                if (!perPageOptionsStatus) {
                    scope.conf.perPageOptions.push(scope.conf.itemsPerPage);
                }

                // 对选项进行sort
                scope.conf.perPageOptions.sort(function (a, b) { return a - b });

                /**begin 获取分页页数数据**/
                scope.pageList = [];
                if (scope.conf.numberOfPages <= scope.conf.pagesLength) {
                    // 判断总页数如果小于等于分页的长度，若小于则直接显示
                    for (i = 1; i <= scope.conf.numberOfPages; i++) {
                        scope.pageList.push(i);
                    }
                } else {
                    // 总页数大于分页长度（此时分为三种情况：1.左边没有...2.右边没有...3.左右都有...）
                    // 计算中心偏移量
                    var offset = (scope.conf.pagesLength - 1) / 2;
                    //左边没有
                    if (scope.conf.currentPage <= offset) {
                        // 左边没有...
                        for (i = 1; i <= offset + 1; i++) {
                            scope.pageList.push(i);
                        }
                        scope.pageList.push('...');
                        scope.pageList.push(scope.conf.numberOfPages);
                    } else if (scope.conf.currentPage > scope.conf.numberOfPages - offset) {
                        scope.pageList.push(1);
                        scope.pageList.push('...');
                        for (i = offset + 1; i >= 1; i--) {
                            scope.pageList.push(scope.conf.numberOfPages - i);
                        }
                        scope.pageList.push(scope.conf.numberOfPages);
                    } else {
                        // 最后一种情况，两边都有...
                        scope.pageList.push(1);
                        scope.pageList.push('...');

                        for (i = Math.ceil(offset / 2) ; i >= 1; i--) {
                            scope.pageList.push(scope.conf.currentPage - i);
                        }
                        scope.pageList.push(scope.conf.currentPage);
                        for (i = 1; i <= offset / 2; i++) {
                            scope.pageList.push(scope.conf.currentPage + i);
                        }

                        scope.pageList.push('...');
                        scope.pageList.push(scope.conf.numberOfPages);
                    }
                }

                //重新绑定
                if (scope.conf.onChange) {
                    scope.conf.onChange();
                }
                scope.$parent.conf = scope.conf;
            }
            /** end pageList数组**/

            // prevPage 上一页
            scope.prevPage = function () {
                if (scope.conf.currentPage > 1) {
                    scope.conf.currentPage -= 1;
                }
            };
            // nextPage 下一页
            scope.nextPage = function () {
                if (scope.conf.currentPage < scope.conf.numberOfPages) {
                    scope.conf.currentPage += 1;
                }
                
            };

            // 跳转页
            scope.jumpToPage = function () {
                scope.jumpPageNum = scope.jumpPageNum.replace(/[^0-9]/g, '');
                if (scope.jumpPageNum !== '') {
                    scope.conf.currentPage = scope.jumpPageNum;
                }
            };

            // 修改每页显示的条数
            scope.changeItemsPerPage = function () {
            };

            //排序
            scope.$parent.orderby = function (_orderby) {
                var torderby = scope.conf.orderBy
                var fieldorderby;
                if (_orderby == torderby) {
                    if (torderby.indexOf('-') > -1) {
                        fieldorderby = _orderby.substring(0, _orderby.length);
                    } else {
                        fieldorderby = '-' + _orderby;
                    }
                }
                else if (torderby) {
                    if (torderby.indexOf('-') > -1) {
                        fieldorderby = _orderby.substring(0, _orderby.length);
                    } else {
                        fieldorderby = '-' + _orderby;
                    }
                } else {

                    fieldorderby = _orderby;
                }
              // var $orderby = (scope.conf.orderBy == _orderby || scope.conf.orderBy == ('-' + _orderby)) ? (scope.conf.orderBy == _orderby ? ('-' + _orderby) : _orderby) : _orderby;
               //console.log(fieldorderby + " |  " + $orderby);
              scope.conf.orderBy = $orderby;
            };

            //监控
            scope.$watch(function () {
                var newValue = scope.conf.currentPage + ' ' + scope.conf.totalItems + ' ' + scope.conf.orderBy + ' ' + scope.conf.query + ' ';

                newValue += scope.conf.itemsPerPage;

                return newValue;

            }, getPagination);

        }
    };
}]);


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




