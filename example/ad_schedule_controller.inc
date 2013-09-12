<?php
    /**
     * ad_schedule_controller 提供广告排期表管理。
     *
     * @package       ad.controllers
     * @version       1.0
     *
     * @create        2013-07-02
     * @modify        2013-07-02
     */
    Jtita::import('ad.models.ad_position');
    Jtita::import('ad.models.ad_ad');
    /**
     * IdleAdPositionController 声明广告排期表控制器
     */
    class AdScheduleController extends AdminsController {
        public $filters = array(
            'position' => array('class'=>'controllers.filters.paginate_filter', 'finder'=>'positions', 'size'=>2)
        );

        public function indexAction() {
            $time = date('Y-m', time());
            $month = explode('-',$time);

            $this->yearMonth = $time;
            $this->month = array_pop($month);
            $this->positions = AdPosition::findAll();
        }

        public function selectAction() {
            $this->autoRender = false;
            $cond['AdAd.status'] = array(AdAd::STATUS_WAIT_THROWIN, AdAd::STATUS_NOW_THROWIN);

            if($this->isPost()) {
                /*
                 * 判断是否是点击排期表查看详情
                 */
                if($this->forms('details')==='true') {
                    $date = $this->forms('date');
                    $position = $this->forms('position_id');

                    $cond['AdAd.start <='] = strtotime($date);
                    $cond['AdAd.end >='] = strtotime($date);
                    $cond['AdAd.position_id'] = intval($position);

                    $results = AdAd::findAll($cond, array('joins'=>'adcustomer'))
                        ->fields('AdAd.name, AdCustomer.name as cName, AdAd.start, AdAd.end, AdAd.status')
                        ->rows();

                    echo json_encode($results);
                }
                else {
                    /**
                     * 查找符合条件的广告并填充至表格
                     */
                    $results = array();
                    $ads = array();
                    # 接收要查找时期的当前年和月
                    $yearMonth = $this->forms('year').'-'.$this->forms('month');
                    # 当前月的最大天数
                    $currMonthMaxDay = date('t', strtotime($yearMonth));
                    $day = $this->forms('day')? '-'.$this->forms('day') : '-1';

                    # 判断是否以选择日期范围查找
                    if($this->forms('selectView') === 'true') {
                        $startArr = explode('-', $this->forms('start'));
                        $cond['AdAd.start <'] = strtotime($this->forms('start'));
                        $cond['AdAd.end >='] = strtotime($this->forms('end'));
                        $yearMonth = $startArr[0].'-'.$startArr[1];
                    }
                    else {
                        $cond['AdAd.start <'] = strtotime($yearMonth.'-'.$currMonthMaxDay);
                        $cond['AdAd.end >='] = strtotime($yearMonth.$day);
                    }
                    
                    # 查询数据库记录并取出结果集
                    $positions = AdAd::findAll($cond, array('joins'=>'adposition'))
                        ->fields('AdAd.position_id, AdAd.start as AdStart, AdAd.end as AdEnd, AdPosition.name as ApName,
                                      AdPosition.layout_width, AdPosition.layout_height, AdPosition.ad_num')
                        ->rows();

                    if($positions) {
                        foreach($positions as $arr) {
                            $results[$arr['position_id']] = array(
                                'name'=>$arr['ApName'],
                                'width'=>$arr['layout_width'],
                                'height'=>$arr['layout_height'],
                                'adNum'=>$arr['ad_num'],
                                'adData'=>array()
                            );

                            $ads[] = array('posId'=>$arr['position_id'], 'start'=>$arr['AdStart'], 'end'=>$arr['AdEnd']);
                            $resultArray[$arr['position_id']] = $this->_schedule($ads, $yearMonth, $day, $currMonthMaxDay);
                        }

                        foreach($resultArray as $key => $arr) {
                            foreach($arr as $posId => $row) {
                                if($key == $posId) {
                                    array_push($results[$posId]['adData'], $row);
                                }
                            }
                        }
                    }

                    echo json_encode($results);
                }
            }
        }

        /**
         * 处理排期表
         *
         * @param array $arr
         * @param string $yearMonth
         * @param int $day
         * @param int $currMonthMaxDay
         * @return array
         */
        private function _schedule($arr, $yearMonth, $day, $currMonthMaxDay) {
            $days = array();
            $yearMonthArr = explode('-', $yearMonth);

            if(is_array($arr) && !empty($arr)) {
                foreach($arr as $row) {
                    $currStartTime = explode('-', date('Y-n-d', $row['start']));
                    $currEndTime = explode('-', date('Y-n-d', $row['end']));
                    $currMonth = explode('-', date('Y-n-d', time()));
                    $currPosition = $row['posId'];

                    # 判断并记录最小开始日期
                    if(isset($lastStart)) {
                        if($row['start'] < $lastStart) {
                            $str = implode('-',$currStartTime);
                        }
                    }
                    else {
                        $str = implode('-',$currStartTime);
                    }

                    # 判断是否从1号还是从本月当前天数开始填充
                    if( ($row['start'] <= strtotime($yearMonth.$day)) &&
                        ($row['end'] >= strtotime($yearMonth.$day)) &&
                        ($currStartTime[1] != $yearMonthArr[1]) ) {
                        $currNum = 1;
                    }
                    else {
                        $currNum = $this->_startTimeInMonth($row['start'], $row['end'], $yearMonth, $currMonthMaxDay);
                    }

                    $daysArr = isset($days[$currPosition][$yearMonthArr[1]]) ? $days[$currPosition][$yearMonthArr[1]] : $days[$currPosition][$yearMonthArr[1]] = array();
                    # 记录上线所在的天数
                    if($yearMonthArr[1] < $currEndTime[1]) {
                        for($num = $currNum; $num <= $currMonthMaxDay; $num++) {
                            if(array_key_exists($num, $daysArr)) {
                                $days[$currPosition][$yearMonthArr[1]][$num] = $days[$currPosition][$yearMonthArr[1]][$num]+1;
                            }
                            else {
                                $days[$currPosition][$yearMonthArr[1]][$num] = 1;
                            }
                        }
                    }
                    else {
                        for($num = $currNum; $num <= $currEndTime[2]; $num++) {
                            if(array_key_exists($num, $daysArr)) {
                                $days[$currPosition][$yearMonthArr[1]][$num] = $days[$currPosition][$yearMonthArr[1]][$num]+1;
                            }
                            else {
                                $days[$currPosition][$yearMonthArr[1]][$num] = 1;
                            }
                        }
                    }

                    # 记录上次的开始时间
                    $lastStart = $row['start'];
                }

                return $days;
            }
        }

        /**
         * 判断开始日期是否在当前月, 并且结束天数大于等于当前月最大天数
         * if istrue
         *  以当前天数作为开始表格的填充基数
         * else
         *  以当前记录的开始时间天数作为开始表格的填充基数
         *
         * @return int
         */
        private function _startTimeInMonth($start, $end, $yearMonth, $currMonthMaxDay) {
            $currStartTime = explode('-', date('n-j', $start));
            $currMonthDay = explode('-', date('n-j', time()));

            if( $start <= strtotime($yearMonth.'-'.$currMonthMaxDay) &&
                $start >= strtotime($yearMonth.'-1') &&
                $end   >= time() &&
                ($currStartTime[0] === $currMonthDay[0] && $currMonthDay[1] >= $currStartTime[1]))
            {
                $currDay = date('j', time());
                return $currDay;
            }
            else {
                return date('j', $start);
            }
        }

        /**
         * 下载排期表
         */
        public function exportAction() {
            $this->autoRender = false;

            # 表格转换为csv格式数组
            $results = $this->tableToCsv($this->forms('table'));

            array_shift($results);
            $file = JTPATH_APP.DIRECTORY_SEPARATOR.'publics'.DIRECTORY_SEPARATOR.'schedule.csv';
            $csvHandler = fopen($file, 'w');

            $k = 0;
            $processResults = array();
            foreach($results as $key => $value) {
                if(empty($value)) {
                    array_splice($results, $key, 2);
                    break;
                }
                $k = $key;
                $processResults[] = iconv("utf-8","gb2312", $value);
            }
            $results[$k+1] = ','.$results[$k+1];

            # 将上面处理后的'干净'数组，连接到对应的广告位
            for($i=$k+1, $key=0; $key<=$k; $key++, $i++) {
                $processResults[$key] .= $results[$i];
            }

            # 写入到CSV文件
            foreach ($processResults as $line) {
                fputcsv($csvHandler, explode(',', $line));
            }
            fclose($csvHandler);
        }

        /**
         * 将表格转为csv格式数组
         *
         * @param string $table
         * @return array
         */
        function tableToCsv($table) {
            $table = preg_replace("'<td class=\"parts-sell\"[^>]*?>'si",'part',$table);
            $table = preg_replace("'<td class=\"all-sell\"[^>]*?>'si",'all',$table);
            $table = str_replace("</td>",',',$table);
            $table = str_replace("</tr>","{tr}",$table);
            # 去HTML 标记
            $table = preg_replace("'<[\/\!]*?[^<>]*?>'si","",$table);
            # 去空白字符
            $table = preg_replace("'([\r\n])[\s]+'","",$table);
            $table = str_replace(" ","",$table);
            $table = str_replace(" ","",$table);
            $table = str_replace("&nbsp;","",$table);
            $table = explode("{tr}",$table);
            array_pop($table);

            return $table;
        }

    }
