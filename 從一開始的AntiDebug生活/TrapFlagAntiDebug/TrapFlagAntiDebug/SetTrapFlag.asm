.CODE
SetTrapFlag PROC
    ; �� Flag �̥�� Stack �W
	pushf

    ; �]�w Trap Flag�A��m�b�� 9 �� bit�A�ҥH�O 0x100
    or dword ptr [rsp], 100h

    ; ��s�� Flag �q Stack ���� Flag ��
    popf
    nop
    ret
SetTrapFlag ENDP
END