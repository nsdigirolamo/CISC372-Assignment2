#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

typedef enum {CLUBS=0,SPADES=1,HEARTS=2,DIAMONDS=3} SUIT;

typedef struct _card {
	int rank;
	SUIT suit;
} Card;
typedef Card Hand[5];

char* labels[4]={"clubs","spades","hearts","diamonds"};

/*randomCard
 * Description: Generates a random playing card
 * Arguments: card: A pointer to a card object to store the result
 * Returns: None
 */
void randomCard(Card* card){
	int value= rand();
	card->rank=value%13+1;
	card->suit=(SUIT)rand()%4;

}
/*getTotalTrials
 * Description: Gets the total trials from the user and stores it in cnt
 * Arguments: cnt: an output variable to hold the selected number of trials
 * 			rank: the rank of the current process.
 * Returns: None
 */
 void getTotalTrials(int* cnt, int rank){
	int* trial_count = cnt;

	// Process 0 gets the number of trials
	if (rank == 0) {
		printf("Enter the number of trials\n");
		scanf("%d", trial_count);
	}

	/**
	* All processes call Bcast with the source set to 0.
	* In process 0, this well send trial_count to all other processes in MPI_COMM_WORLD.
	* In all other processes, this will populate trial_count with the value sent by process 0.
	* When this function returns, whatever variable was passed for trial_count will contain
	* the input value in all processes.
	* 
	* trial_count is the data buffer
	* 1 is the size of the data buffer (it's just a count)
	* MPI_INT is the data type of the buffer
	* 0 is the process sending the data
	* MPI_COMM_WORLD is the communicator of the threads involved.
	*/

	MPI_Bcast(trial_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
 }

/*inHand
 * Description: checks if a card is in the hand so far
 * Arguments: card: A pointer to a card object to check against the hand
 * 	      hand: An array of cards representing the hand
 * 	      cardCount: The number of valid cards in the hand so far
 * Returns: non-zero if card is in the hand, zero otherwise
 */
int inHand(Card* card, Hand hand,int cardCount){
	for (int i=0;i<cardCount;i++){
		if (card->rank==hand[i].rank && card->suit==hand[i].suit)
			return -1;
	}
	return 0;
}

/*isStraightFlush
 * Description: Checks if the hand is a straight flush
 * Arguments: hand: The hand to check (array of cards)
 * Returns: non-zero if hand represents a straight flush, zero otherwise
 */
int isStraightFlush(Hand hand){
	int swap;
	//first check flush
	SUIT suit=hand[0].suit;
	for (int i=1;i<5;i++)
		if (hand[i].suit!=suit)
			return 0;
	for (int i=0;i<4;i++)
		for (int j=0;j<4-i;j++){
			if(hand[j].rank>hand[j+1].rank){
			       swap=hand[j].rank;	
			       hand[j].rank=hand[j+1].rank;
			       hand[j+1].rank=swap;
			}
		}
	if (hand[4].rank==hand[3].rank+1 && hand[3].rank==hand[2].rank+1 && hand[2].rank==hand[1].rank+1 && (hand[1].rank==hand[0].rank+1 || (hand[0].rank==1 && hand[4].rank==13))){
#ifdef DEBUG
		printf("Found a straight flush!\n");
#endif
		return -1;
	}

	return 0;
}

/*printHand
 * Description: prints out a hand
 * Arguments: hand: Array of card objects to print out
 * Returns: None
 */
void printHand(Hand hand){
		for (int i=0;i<5;i++)
			printf("\t(%d of %s)",hand[i].rank,labels[hand[i].suit]);
		printf("\n");
}

/*makeStraightFlush1
 * Description: Debugging function to generate a straight flush starting at 1 pre-sorted
 * Arguments: hand: An array of cards to store the result in
 * Returns: None
 */
void makeStraightFlush1(Hand hand){
	hand[0].rank=1;
	hand[0].suit=SPADES;
	hand[1].rank=2;
	hand[1].suit=SPADES;
	hand[2].rank=3;
	hand[2].suit=SPADES;
	hand[3].rank=4;
	hand[3].suit=SPADES;
	hand[4].rank=5;
	hand[4].suit=SPADES;
}

/*makeStraightFlush2
 * Description: Debugging function to generate a straight flush starting at 4 unsorted
 * Arguments: hand: An array of cards to store the result in
 * Returns: None
 */
void makeStraightFlush2(Hand hand){
	hand[0].rank=4;
	hand[0].suit=SPADES;
	hand[1].rank=8;
	hand[1].suit=SPADES;
	hand[2].rank=7;
	hand[2].suit=SPADES;
	hand[3].rank=6;
	hand[3].suit=SPADES;
	hand[4].rank=5;
	hand[4].suit=SPADES;
}

/*makeStraightFlush3
 * Description: Debugging function to generate a straight flush starting at 10 unsorted
 * Arguments: hand: An array of cards to store the result in
 * Returns: None
 */
void makeStraightFlush3(Hand hand){
	hand[0].rank=11;
	hand[0].suit=SPADES;
	hand[1].rank=13;
	hand[1].suit=SPADES;
	hand[2].rank=12;
	hand[2].suit=SPADES;
	hand[3].rank=10;
	hand[3].suit=SPADES;
	hand[4].rank=1;
	hand[4].suit=SPADES;
}

int main(int argc,char** argv){

	double t1, t2;

	// Creates the world communicator.
	MPI_Init(&argc, &argv);

	// Retrieves the process count from the world communicator.
	int process_count;
	MPI_Comm_size(MPI_COMM_WORLD, &process_count);

	// Retrieves the rank of the current process within the world communicator.
	// Each process calls this method and gets a different number for its rank.
	int current_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &current_rank);

	int localStraightFlushes = 0;
	Hand pokerHand;
	srand(time(NULL) + current_rank);
	int trial_count;
	getTotalTrials(&trial_count, current_rank);

	int trials_to_do = trial_count / process_count;
	int leftover_trials = trial_count % process_count;
	trials_to_do += current_rank < leftover_trials ? 1 : 0;

#ifdef DEBUG
	if (current_rank == 0) {
		printf("leftover_trials: %d\n", leftover_trials);
		printf("(trial_count: %d) / (process_count: %d) = (trials_to_do: %d)\n", trial_count, process_count, trials_to_do);
	}
	if (current_rank < leftover_trials) {
		printf("I am process %d and I have an additional trial to complete!\n", current_rank);
	}
#endif

	if (current_rank == 0) {
		t1 = MPI_Wtime();
	}

	for (int i = 0; i < trials_to_do; i++) {
		int cardCount=0;
		while (cardCount<5){
			Card card;
			randomCard(&card);
			if (!inHand(&card,pokerHand,cardCount)){
				pokerHand[cardCount].rank=card.rank;
				pokerHand[cardCount].suit=card.suit;
				cardCount++;
			}
		}
		if (isStraightFlush(pokerHand))
			localStraightFlushes++;
	}

	int globalStraightFlushes = 0;
	MPI_Reduce(&localStraightFlushes, &globalStraightFlushes, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

	if (current_rank == 0) {
		t2 = MPI_Wtime();
	}

	if (current_rank == 0) {
		float percent = ((float)(globalStraightFlushes) / (float)(trial_count)) * 100.0;
		printf("We found %d straight flushes out of %d hands or %f percent.\n", globalStraightFlushes, trial_count, percent);
		printf("Completed the main loop and reduced the results in %f seconds.\n", t2 - t1);
	}

	MPI_Finalize();

	return 0;
}
