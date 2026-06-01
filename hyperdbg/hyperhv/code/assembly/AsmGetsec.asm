PUBLIC AsmGetsecCapabilities

.code _text

;------------------------------------------------------------------------

AsmGetsecCapabilities PROC PUBLIC

	PUSH RBX		; Save state

	XOR RBX, RBX	; Clear RBX to get result
	XOR RAX, RAX	; Clear RAX to specify the GETSEC leaf (0)
	
	GETSEC

	POP RBX			; Restore state

	RET

AsmGetsecCapabilities ENDP

;------------------------------------------------------------------------

END