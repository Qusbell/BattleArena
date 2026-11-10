---
name: grilling
description: Grill the user relentlessly about a plan, decision, or idea. Use when the user wants to stress-test their thinking, or uses any 'grill' trigger phrases.
---

Interview the user relentlessly until you reach a shared understanding. Map this as a **design tree**: every decision branches into the decisions that hang off it.

Work the tree in **rounds**. The **frontier** is every decision whose prerequisites are already settled: the questions you can ask *now* without guessing at answers you haven't heard yet. Ask the whole frontier in one round: number each question and give your recommended answer. Then wait for the user's answers before the next round.

Use `request_user_input_async` to present interactive multiple-choice questions.

Word each question so "yes" accepts your recommended answer.

Each round the user answers reshapes the tree: settled decisions push the frontier outward and unblock questions that depended on them. Recompute the frontier and ask the next round. A question whose answer depends on another question still open in this round belongs to a *later* round, not this one.

Finding *facts* is your job, never the user's. When a frontier question needs a fact from the environment (filesystem, tools, etc.), investigate it directly using available tools; don't ask the user for anything you could look up yourself. Don't block unrelated questions on the investigation: an unfinished exploration is an unsettled prerequisite, so only the questions downstream of it must wait. Ask the rest of the frontier now using `request_user_input_async`.

The session is done when the frontier is empty: every branch of the design tree visited, nothing left silently assumed. Do not act on it until the user confirms you have reached a shared understanding.

The grilling session ends once shared understanding is confirmed. Then use `request_user_input_async` to offer:

1. **Create Decision & Implement (Recommended)** — Save to `Decision/<number>-<document-name>.md`, then start implement.
2. **Implement Only** — Start implement without creating a Decision document.
3. **Other** — Specify an alternative.

Do not act until the user selects an option.
