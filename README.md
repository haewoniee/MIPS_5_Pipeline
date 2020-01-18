# MIPS 5-Stage (RISC CPU) 파이프라인 시뮬레이션 2018.08

시스템 구조론 수업에서 MIPS 5-스테이지 파이프라인 시뮬레이터를 구현하였습니다.

    Usage:
        $ make
        $ ./asm input.s input.out #어셈블리를 기계언어로 번역
        $ ./sim input.out > input_s.output #파이프라인 없는 버전으로 input명령들 수행
        $ ./sim-pipe input.out > input_m.output #파이프라이닝 버전으로 input명령들 수행

---

## <프로젝트 개요>

- 개발인원: 1명 (본인)
- 개발기간: 2018.08.07 ~ 2020.08.14 (7일)
- 개발환경: 리눅스 터미널
- 개발언어: C
- 세부사항:
    - 32-bit 아키텍쳐 (레지스터와 데이터 모두 32bit로 취급)
    - 3가지 타입의 명령어 input
        - I-type: LW, SW, BEQZ, ADDI
        - R-type: ADD, SUB, SLL, SRL, AND, OR
        - J-type: HALT

---

## <구현 부분(**mips-small-pipe.c**)> - 아래 그림 참조

<img src="https://s3.us-west-2.amazonaws.com/secure.notion-static.com/33b22f9f-7228-40a9-8a37-ecf2a570a5f3/Untitled.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIAT73L2G45HLK2ILT4%2F20200118%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200118T122925Z&X-Amz-Expires=86400&X-Amz-Security-Token=IQoJb3JpZ2luX2VjEDIaCXVzLXdlc3QtMiJGMEQCIGd%2BOmupZLk0b5jQ3Rb99VrNyk6Pw%2BqzaFhvwabuk46SAiB4nZR3Z7ZRwOojPcckt1lvlGgEoE3kkB%2BUQJ6WeNCPhSq9Awi7%2F%2F%2F%2F%2F%2F%2F%2F%2F%2F8BEAAaDDI3NDU2NzE0OTM3MCIMXxvxI23TIySZYw8cKpEDlTASVrO8QzZF1b83GrFUs%2FwJxybx4sMVe%2FoswkAuVnl%2BB1RevVj8vRrp%2FlFHGDj%2FFFW0X1wqPBFisAm%2BN2xPNbQf6BR7%2BCROhKbc3UHEONNydTLesUpeaK1ii%2BDcmmD9tbRtDNxnPEQVaLJGYd2R1lcC39QNF9%2F09Qwyb61HVpnWYaKCiDa19aBQ5cLqGMk9OHNotJ3RFxiPnrh1jDaUvl9Z1mlO9o6f9upX%2FasTqgwuXwCq8PixI6%2FSpsXga03v%2B0ne58djl9XfNMwbBrQBTJxFlcxvapYq4pKmA2%2FjPgjgpJqdku%2BnOX1jbjhWea9ZMIYyRkxsYdCeWo8YOuxaJ2UTP%2BUWg4hU%2BYzivYskAmRZLesyeCzYtf3R1yRGsy0aWKBWPuWkmFsZ6dOshQS7%2FUg7Z4rtjWHHfXaW2ldkfE2%2FMkmWmqATu0fg%2BSmKPiNOndE%2B4fSO0EcVgQQgy82TBF5gyqS9PpWwym2NLlU6u4IGS6PO2F7HI7P4lahvo7FwguMRw12zNXe6uThDANxa0M4w0a6L8QU67AG0Ws%2BaB0QhGm3g9iIRh2Kkto7rGsKXPk9F8vSbLtbU7QUMV6r%2Bcg9M4asveevrzyW8BQQ1xI08KvW3fHr8885khqpnOBYHODfYC1sMGcXyK6qnbheAsnwIdh63NU%2BQgWUOrefk%2F2kDZ7dTJTGe8UiiL2aXYRsFQWdD04TaIqmqA0wkMB1x4%2Fb%2FD3Npd53WIH4zJtfty%2FAoihgUNjNFwJjGyAid%2BI%2BHZe%2BSP9WUo7ZJVrDBRLnCXUlebM8mhKSaB8r3gznSNzidbaHiiFUXyvrP0z3pe%2FrT4jBh2EvegHD2eYqbWUYU%2Bg3k3vigxg%3D%3D&X-Amz-Signature=cdbdfe9bd62a513b4f6cb582223afbd0503b07c67a73964887b7fcee83a6219a&X-Amz-SignedHeaders=host&response-content-disposition=filename%20%3D%22Untitled.png%22" width="90%"></img>

**5-Stage MIPS Pipeline** (Figure C.22 @ Computer architecture: a quantitative approach by Hennessy & Patterson)

- IF stage - 메모리에서 명령어 적재
    - 현재 PC를 참조하여 명령어를 4비트만큼 레지스터에 적재 (IR)
    - 읽어들인 명령어 길이인 4만큼 PC 만큼 더하고(MUX), NPC(newPC)에 저장.
    - 데이터 지정 명령어 일 경우 16비트만큼 레지스터에 적재 (Imm)
- ID stage - 명령어 해석, 레지스터 읽기
    - 명령어 해석 후 레지스터에서 필요한 값들을 읽어들여 새로운 임시 레지스터에 저장
- EX stage  - 명령어 실행, 주소 계산
    - 명령어에 따라 계산(MUX) 실행 후 결과를 임시 레지스터에 저장
    - 이번 시뮬레이터의 경우: EX스테이지 후 MEM 스테이지로 넘어가기 전 "branch taken" 시그널을 보내는 대신, 다음 branch 주소를 계산 후 PC 레지스터를 직접적으로 바꿈
- MEM stage - 메모리 접근
    - PC를 NPC로 업데이트
    - 피연산자의 메모리에 접근하고 데이터를 작성.
    - 이번 시뮬레이터의 경우: 다시 메모리에서 데이터를 읽어올 수 있는지 확인하는 과정을 WB단계를 가기 전 MEM단계에서 수행.
- WB stage - 레지스트리에 결과값 기록
    - 결과를 다시 메모리에서 레지스터에 기록.
    - 성공하지 못할 경우 ID단계로 돌아가지만, 이번 시뮬레이터의 경우 이 단계가 MEM스테이지로 돌아감.

---

# 성과

## <파이프라이닝이 없는 프로세서인 mips-small.c와의 비교>

<article id="7f5f6f1d-1b1f-47c9-a94f-eeac6e636994" class="page sans"><header><h3 class="page-title">독립 연산 VS 파이프라이닝: 수행 사이클 수 비교</h3></header><div class="page-body"><table class="collection-content"><thead><tr><th>명령어</th><th>독립 연산</th><th>파이프라이닝</th></tr></thead><tbody><tr id="e3550a9f-df26-42e6-952b-0583253c7049"><td class="cell-title">덧셈 후 뺄셈</td><td class="cell-v4j:">40</td><td class="cell-UR(O">10</td></tr><tr id="793f5431-7147-47e2-8d03-e8121f9979b5"><td class="cell-title">곱셈(229 * 123)</td><td class="cell-v4j:">350</td><td class="cell-UR(O">79</td></tr><tr id="047186b1-3330-42f0-9576-a861760c172f"><td class="cell-title">포워딩이 있는 덧뺄셈</td><td class="cell-v4j:">60</td><td class="cell-UR(O">17</td></tr><tr id="efa49cf3-d103-4d07-b032-a4190f502b80"><td class="cell-title">브랜칭 (I-type)</td><td class="cell-v4j:">90</td><td class="cell-UR(O">19</td></tr><tr id="3f9cc77a-6723-4e47-bcca-c1fc29d3a320"><td class="cell-title">곱셈(123456789^2) - 아래 코드 참고</td><td class="cell-v4j:">335</td><td class="cell-UR(O">82</td></tr></tbody></table></div>

**mult_big.s : 123456789 * 123456789를 수행하는 어셈블리 파일**

    add	1	0	0	r1 <- result=0
    	lw	2	0	mplier	r2 <- multiplier
    	lw	3	0	mcand	r3 <- multiplicand
    	addi	5	0	1	r5 <- check bit
    	addi	6	0	8	r6 <- index (starts at 8)
    	addi	7	0	-1	r7 <- -1
    	addi	8	0	1	r8 <- constant 1 for shifting
    loop	and	4	2	5	see if current bit of mplier==1
    	beqz	0	4	skip	if bit is 0, skip the add
    	add	1	3	1	add current multiplicand to result
    skip	sll	3	3	8	shift mcand left 1 bit by doubling it
    	sll	5	5	8	shift check left 1 bit by doubling it
    	add	6	7	6	decrement index
    	beqz	0	6	end	check if done
    	beqz	0	0	loop	jump back to loop
    end	sw	1	0	answer
    	halt
    mcand	.fill	123456789			multiplicand
    mplier	.fill	123456789			multiplier
    answer	.fill	0			answer

*주어진 어셈블리인 asm.c로 위 파일 및 다른 어셈블리 파일들을 머신코드로 변환하여 사용하였습니다.
