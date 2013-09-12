<?php
    /**
     * ad_schedule_controller 提供广告排期表管理。
     *
     * @copyright     2012-2013 jtita.com (http://www.jtita.com)
     * @author        heshuang
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
            if($this->isPost()) {
                if($this->forms('details')==='true') {
                    $date = $this->forms('date');
                    $position = $this->forms('position_id');

                    $cond['AdAd.start <='] = strtotime($date);
                    $cond['AdAd.end >='] = strtotime($date);
                    $cond['AdAd.position_id'] = intval($position);

                    $results = AdAd::findAll($cond, array('joins'=>'adcustomer'))->fields('AdAd.name, AdCustomer.name as cName, AdAd.start, AdAd.end')->rows();

                    echo json_encode($results);
                }
                else {
                    $results = array();
                    $ads = array();
                    $yearMonth = $this->forms('year').'-'.$this->forms('month');
                    $currMonthMaxDay = date('t', strtotime($yearMonth));
                    $day = $this->forms('day')? '-'.$this->forms('day') : '-1';

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

                    $this->positions = AdAd::findAll($cond, array('joins'=>'adposition'))
                        ->fields('AdAd.position_id, AdAd.start as AdStart, AdAd.end as AdEnd, AdPosition.name as ApName,
                                      AdPosition.layout_width, AdPosition.layout_height, AdPosition.ad_num')
                        ->rows();

                    if($this->positions) {
                        foreach($this->positions as $arr) {
                            $results[$arr['position_id']] = array(
                                'name'=>$arr['ApName'],
                                'width'=>$arr['layout_width'],
                                'height'=>$arr['layout_height'],
                                'adNum'=>$arr['ad_num'],
                                'adData'=>array()
                            );

                            $ads[] = array('posId'=>$arr['position_id'], 'start'=>$arr['AdStart'], 'end'=>$arr['AdEnd']);
                            $resultArray[$arr['position_id']] = AdAd::schedule($ads, $yearMonth, $day, $currMonthMaxDay);
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

    }