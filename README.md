# MIPS 5-Stage (RISC CPU) 파이프라인 시뮬레이션 2018.08

시스템 구조론 수업에서 MIPS 5-스테이지 파이프라인 시뮬레이터를 구현하였습니다.

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

![MIPS%205%20Stage%20RISC%20CPU%202018%2008/Untitled.png](MIPS%205%20Stage%20RISC%20CPU%202018%2008/Untitled.png)

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

[독립 연산 VS 파이프라이닝: 수행 사이클 수 비교](https://www.notion.so/7f5f6f1d1b1f47c9a94feeac6e636994)

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
