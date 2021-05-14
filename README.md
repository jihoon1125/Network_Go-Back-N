# Network_Go-Back-N
Go Back N 프로토콜을 직접 구현하면서 적절한 timeout interval에 대해 고찰해보고 TCP 통신의 매커니즘을 익힌다.

---

## Bidirectional Go Back N FSM

![image](https://user-images.githubusercontent.com/67624104/118277956-5bd34080-b504-11eb-8a0d-d05e06c4bc3d.png)

* Input || call from above
  * Output 함수가 호출된다.
  * 패킷의 버퍼가 window size를 넘지 않는다면 패킷들을 전송할 수 있다.
  * Pure data를 전송해야 하는 경우 ack를 999로 설정한다.(Sender 입장)
  * Ack을 piggybacking 방식으로 전송해야 하는 경우 ack을 expectedseqnum으로 설정(Receiver 입장)
  * Base가 nextseqnum과 같다면 timer 시작

* Notcorrupt
  * Checksum 값이 맞다면 state 1 
  * 만약 seqnum이 expectedseqnum과 같다면 패킷으로부터 데이터 추출하고 expectedseqnum++
  * Acknum이 999가 아니면 ack를 piggybacking으로 전송해야함
  * Acknum이 base보다 작으면 중복 ack이라는 뜻으로 무시한다
  * Base가 nextseqnum이 되지 않았다면 timer restart 필요

* Timeout
  * Timeout 되면 timer_interrupt 호출. Timer restart하고 base~nextseqnum-1까지 재전송하도록 한다.

---

## Result image

![image](https://user-images.githubusercontent.com/67624104/118280803-807ce780-b507-11eb-8891-4be2686101dd.png)


