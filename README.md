# CSE 220 Final Project

## BY Lee-Way Wang and Octavio Villalobos

The technique that we are implementing is SWQUE: A Mode Switching Issue Queue with Priority-Correcting Circular Queue, originally proposed by Hideki Ando.

SWQUE (switch issuing queue) is a technique created by Hideki Ando aimed at improving single-thread performance by modifying the already issue queue (AGE technique) and changing it to a combination of both a CIRC-PC (circular queue) and AGE (random queue with an age matrix). The goal of this technique is to achieve correct prioritization of instruction and high capacity efficiency. Regarding lower capacity phases, which indicate they are more priority-sensitive, SWQUE will utilize the modified CIRC-PC. When it is in a more capacity-demanding phase, meaning we have many instructions, it will operate in the typical AGE technique (Ando 506).


### Project Checkin

Curently a majority of the work is being done at the moment within the UOP_QUEUE_STAGE.CC file within the Scarab source code, we are currently mapping things out still so we have written comments with what needs to go where just to have a frame work to begin with. We have also made a another repo with our circular queue implementation, please check out our [GitHub](https://github.com/leeway64/CSE-220-final-project-proof-of-concept) to see the proof of concept.
