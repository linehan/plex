/* 
 * Make a DFA transition table from an NFA created with 
 * Thompson's construction.
 */

/* 
 * DTAB is the deterministic transition table. It is indexed by state number
 * along the major axis and by input character along the minor axis. Dstates
 * is a list of deterministic states represented as sets of NFA states.
 * NSTATES is the number of valid entries in DTAB.
 */
struct dfa_state {
        unsigned group:8; // Group id used by minimize()
        unsigned mark:1;  // Mark used by make_dtran()
        char *accept;     // accept action if accept state
        int anchor        // Anchor point if an accept state.
        SET *set;         // Set of NFA states represented by this DFA state
};


struct dfa_state *DSTATES;     // DFA states table	
struct dfa_state *PREV_STATE;  // Most-recently marked DFA state in DTAB

ROW *DTAB;                     // DFA transition table
int NSTATES                    // Number of DFA states



/**
 * dfa
 * ```
 * Turn an NFA with indicated start state (sstate) into a DFA and
 * return the number of states in the DFA transition table.
 * 
 * dfap is modified to point at that transition table
 * acceptp is modified to point at an array of accepting states
 * (indexed by state number).
 * dfa() discards all the memory used for the initial NFA.
 */
int dfa(FILE *input, ROW *(dfap[]), ACCEPT *(*acceptp))
{
        ACCEPT *accept_states;
        int start
        int i;

        /* Build the NFA */
        start = nfa(input);

        NSTATES    = 0;
        DSTATES    = calloc(DFA_MAX, sizeof(DFA_STATE));
        DTAB       = calloc(DFA_MAX, sizeof(ROW));
        PREV_STATE = DSTATES;

        if (!DSTATES || !DTAB)
	        halt(SIGABRT, "No memory for DFA transition matrix!");

        /* Convert NFA to DFA */
        make_dtran(start);

        /* Free the NFA memory, but not the accept strings. */
        free_nfa(); 

        /* Build the DFA */
        DTAB = realloc(DTAB, NSTATES * sizeof(ROW));
        accept_states = malloc(NSTATES * sizeof(ACCEPT));

        if (!accept_states || !DTAB)
	        halt(SIGABRT, "Out of memory!!");

        for (i=NSTATES; i-->0;) {
	        accept_states[i].string = DSTATES[i].accept;
	        accept_states[i].anchor = DSTATES[i].anchor;
        }

        free(DSTATES);
        *dfap    = DTAB;
        *acceptp = accept_states;

        return NSTATES;
}



int add_to_dstates(SET *NFA_set, char *accept_string, int anchor)
{
        int nextstate;

	if (NSTATES > (DFA_MAX-1))
	    halt(SIGABRT, "Too many DFA states\n");

	nextstate = NSTATES++;
	DSTATES[nextstate].set    = NFA_set;
	DSTATES[nextstate].accept = accept_string;
	DSTATES[nextstate].anchor = anchor;

	return nextstate;
}


/** 
 * in_dstates
 * ``````````
 * If there's a set in Dstates that is identical to NFA_set, return the
 * index of the Dstate entry, else return -1.
 */
int in_dstates(SET *NFA_set)
{
        struct dfa_state *p;
        struct dfa_state *end = &DSTATE[NSTATES];

        for (p=DSTATES; p<end; ++p) {
	        if (IS_EQUIVALENT(NFA_set, p->set))
	                return(p - DSTATES);
        }

        return -1;
}


/* 
 * get_unmarked
 * ````````````
 * Return a pointer to an unmarked state in Dstates. If no such state
 * exists, return NULL. Print an asterisk for each state to tell the
 * user that the program hasn't died while the table is being constructed.
 */
DFA_STATE *get_unmarked(void)
{
        for (; Last_marked<&DSTATES[NSTATES]; ++Last_marked) {
	        if (!Last_marked->mark) {
	                putc('*', stderr);
	                fflush(stderr);
	                return Last_marked;
	        }
        }
        return NULL;
}


/* Free the memory used for the NFA sets in all Dstate entries.  */
void free_sets(void)
{
        struct dfa_state *p;
        struct dfa_state *end = &DSTATES[NSTATES];

        for (p=DSTATES; p<end; ++p)
	        delset(p->set);
}


/**
 * make_dtran
 * ``````````
 * @sstate: starting NFA state
 */
void make_dtran(int sstate)
{
        struct dfa_state *current; // state currently being expanded
        SET *NFA_set               // set of NFA states that define next DFA state
        int nextstate;             // goto DFA state for current char
        char *isaccept;            // current DFA state is accept 
        int anchor;                // anchor, if any
        int c;                     //input char

        /* 
         * Initially Dstates contains a single, unmarked, start state 
         * formed by taking the epsilon closure of the NFA start state. 
         * So, Dstates[0] (and DTAB[0]) is the DFA start state.
         */
        NFA_set = newset() ;
        ADD(NFA_set, sstate);

        NSTATES	= 1;
        DSTATES[0].set  = e_closure(NFA_set,&DSTATES[0].accept,&DSTATES[0].anchor);
        DSTATES[0].mark = 0;

        /* Make the table */
        while (current = get_unmarked()) {

	        current->mark = 1;

	        for (c=MAX_CHARS; --c>=0;) {
	                if (NFA_set = move(current->set, c))
		                NFA_set = e_closure(NFA_set, &isaccept, &anchor);

	                if (!NFA_set) // No outgoing transitions	
		                nextstate = F;

	                else if((nextstate = in_dstates(NFA_set)) != -1)
		                delset(NFA_set);

	                else
		                nextstate = add_to_dstates(NFA_set, isaccept, anchor);

	                DTAB[current-DSTATES][c] = nextstate;
	        }
        }
        /* Terminate string of *'s printed in get_unmarked(); */
        putc('\n', stderr);   	

        /* Free the memory used for the DFA_STATE sets */
        free_sets(); 
}

