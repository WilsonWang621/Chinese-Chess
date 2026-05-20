const SIDE = {
    NONE: "none",
    RED: "red",
    BLACK: "black",
};

const TYPE = {
    EMPTY: "empty",
    GENERAL: "general",
    ADVISOR: "advisor",
    ELEPHANT: "elephant",
    HORSE: "horse",
    CHARIOT: "chariot",
    CANNON: "cannon",
    SOLDIER: "soldier",
};

const ROWS = 10;
const COLS = 9;

const redNames = {
    [TYPE.GENERAL]: "帅",
    [TYPE.ADVISOR]: "仕",
    [TYPE.ELEPHANT]: "相",
    [TYPE.HORSE]: "马",
    [TYPE.CHARIOT]: "车",
    [TYPE.CANNON]: "炮",
    [TYPE.SOLDIER]: "兵",
};

const blackNames = {
    [TYPE.GENERAL]: "将",
    [TYPE.ADVISOR]: "士",
    [TYPE.ELEPHANT]: "象",
    [TYPE.HORSE]: "马",
    [TYPE.CHARIOT]: "车",
    [TYPE.CANNON]: "炮",
    [TYPE.SOLDIER]: "卒",
};

const pieceValue = {
    [TYPE.GENERAL]: 10000,
    [TYPE.CHARIOT]: 500,
    [TYPE.CANNON]: 350,
    [TYPE.HORSE]: 300,
    [TYPE.ELEPHANT]: 120,
    [TYPE.ADVISOR]: 120,
    [TYPE.SOLDIER]: 80,
    [TYPE.EMPTY]: 0,
};

const AI_STRATEGIC_DEPTH = 2;
const AI_STRATEGIC_MOVE_LIMIT = 6;
const AI_TACTICAL_DEPTH = 4;
const AI_TACTICAL_MOVE_LIMIT = 6;
const AI_WIN_SCORE = 1000000;
const AI_MATE_SCORE = 900000;

let board = [];
let turn = SIDE.RED;
let selected = null;
let legalForSelected = [];
let lastMove = null;
let gameOver = false;

const boardEl = document.getElementById("board");
const statusText = document.getElementById("statusText");
const turnBadge = document.getElementById("turnBadge");
const moveList = document.getElementById("moveList");
const resetButton = document.getElementById("resetButton");
const overlayResetButton = document.getElementById("overlayResetButton");
const resultOverlay = document.getElementById("resultOverlay");
const resultTitle = document.getElementById("resultTitle");

function emptyPiece() {
    return { type: TYPE.EMPTY, side: SIDE.NONE };
}

function makePiece(type, side) {
    return { type, side };
}

function resetBoard() {
    board = Array.from({ length: ROWS }, () => Array.from({ length: COLS }, emptyPiece));
    setupBackRank(0, SIDE.BLACK);
    board[2][1] = makePiece(TYPE.CANNON, SIDE.BLACK);
    board[2][7] = makePiece(TYPE.CANNON, SIDE.BLACK);
    for (let col = 0; col < COLS; col += 2) {
        board[3][col] = makePiece(TYPE.SOLDIER, SIDE.BLACK);
    }

    setupBackRank(9, SIDE.RED);
    board[7][1] = makePiece(TYPE.CANNON, SIDE.RED);
    board[7][7] = makePiece(TYPE.CANNON, SIDE.RED);
    for (let col = 0; col < COLS; col += 2) {
        board[6][col] = makePiece(TYPE.SOLDIER, SIDE.RED);
    }

    turn = SIDE.RED;
    selected = null;
    legalForSelected = [];
    lastMove = null;
    gameOver = false;
    moveList.innerHTML = "";
    hideResultOverlay();
    setStatus("红方先行");
    render();
}

function buildConfetti() {
    const confetti = resultOverlay.querySelector(".confetti");
    const colors = ["#ba2029", "#f2b84b", "#147d64", "#2d6cdf", "#8a4bd9", "#f07a2f"];
    confetti.innerHTML = "";
    for (let index = 0; index < 72; index += 1) {
        const piece = document.createElement("i");
        piece.style.setProperty("--x", `${Math.random() * 100}%`);
        piece.style.setProperty("--dx", `${Math.random() * 220 - 110}px`);
        piece.style.setProperty("--r", `${Math.random() * 360}deg`);
        piece.style.setProperty("--d", `${1.8 + Math.random() * 1.6}s`);
        piece.style.setProperty("--delay", `${Math.random() * 0.55}s`);
        piece.style.setProperty("--c", colors[index % colors.length]);
        confetti.appendChild(piece);
    }
}

function setupBackRank(row, side) {
    board[row][0] = makePiece(TYPE.CHARIOT, side);
    board[row][1] = makePiece(TYPE.HORSE, side);
    board[row][2] = makePiece(TYPE.ELEPHANT, side);
    board[row][3] = makePiece(TYPE.ADVISOR, side);
    board[row][4] = makePiece(TYPE.GENERAL, side);
    board[row][5] = makePiece(TYPE.ADVISOR, side);
    board[row][6] = makePiece(TYPE.ELEPHANT, side);
    board[row][7] = makePiece(TYPE.HORSE, side);
    board[row][8] = makePiece(TYPE.CHARIOT, side);
}

function render() {
    boardEl.innerHTML = "";
    renderBoardDecorations();
    for (let row = 0; row < ROWS; row += 1) {
        for (let col = 0; col < COLS; col += 1) {
            const cell = document.createElement("button");
            cell.type = "button";
            cell.className = "cell";
            cell.setAttribute("role", "gridcell");
            cell.dataset.row = String(row);
            cell.dataset.col = String(col);

            const piece = board[row][col];
            const moveHint = legalForSelected.find((move) => samePos(move.to, { row, col }));
            if (piece.side === SIDE.RED && turn === SIDE.RED && !gameOver) {
                cell.classList.add("selectable");
            }
            if (selected && selected.row === row && selected.col === col) {
                cell.classList.add("selected");
            }
            if (moveHint) {
                cell.classList.add(piece.type === TYPE.EMPTY ? "hint" : "capture");
            }
            if (lastMove && samePos(lastMove.from, { row, col })) {
                cell.classList.add("last-move", "last-from");
                cell.dataset.moveLabel = "起";
            }
            if (lastMove && samePos(lastMove.to, { row, col })) {
                cell.classList.add("last-move");
                cell.classList.add("last-to");
                cell.dataset.moveLabel = "落";
            }

            if (piece.type !== TYPE.EMPTY) {
                const pieceEl = document.createElement("span");
                pieceEl.className = `piece ${piece.side}`;
                pieceEl.textContent = pieceName(piece);
                cell.appendChild(pieceEl);
            }

            cell.addEventListener("click", () => handleCellClick(row, col));
            boardEl.appendChild(cell);
        }
    }

    turnBadge.textContent = turn === SIDE.RED ? "红方" : turn === SIDE.BLACK ? "黑方" : "结束";
    turnBadge.classList.toggle("black", turn === SIDE.BLACK || turn === SIDE.NONE);
}

function renderBoardDecorations() {
    const blackPalace = document.createElement("div");
    blackPalace.className = "palace-mark palace-mark-black";
    blackPalace.setAttribute("aria-hidden", "true");
    boardEl.appendChild(blackPalace);

    const redPalace = document.createElement("div");
    redPalace.className = "palace-mark palace-mark-red";
    redPalace.setAttribute("aria-hidden", "true");
    boardEl.appendChild(redPalace);
}

function handleCellClick(row, col) {
    if (gameOver || turn !== SIDE.RED) {
        return;
    }

    const pos = { row, col };
    const piece = board[row][col];

    if (selected) {
        const move = legalForSelected.find((candidate) => samePos(candidate.to, pos));
        if (move) {
            performHumanMove(move);
            return;
        }
    }

    if (piece.side === SIDE.RED) {
        selected = pos;
        legalForSelected = legalMoves(SIDE.RED).filter((move) => samePos(move.from, pos));
        setStatus(`${pieceName(piece)}：选择落点`);
    } else {
        selected = null;
        legalForSelected = [];
        setStatus("请选择红方棋子");
    }

    render();
}

function performHumanMove(move) {
    const captured = board[move.to.row][move.to.col];
    applyMove(move, true, SIDE.RED);
    addMoveLog("红", move, captured);
    selected = null;
    legalForSelected = [];

    if (checkGameEndAfterMove(SIDE.BLACK, "你赢了")) {
        render();
        return;
    }

    turn = SIDE.BLACK;
    setStatus("AI 思考中");
    render();
    window.setTimeout(aiTurn, 260);
}

function aiTurn() {
    if (gameOver) {
        return;
    }

    const moves = legalMoves(SIDE.BLACK);
    if (moves.length === 0) {
        endGame("你赢了，黑方无棋可走");
        render();
        return;
    }

    const move = chooseAiMove(moves);
    const captured = board[move.to.row][move.to.col];
    applyMove(move, true, SIDE.BLACK);
    addMoveLog("黑", move, captured);

    if (checkGameEndAfterMove(SIDE.RED, "AI 获胜")) {
        render();
        return;
    }

    turn = SIDE.RED;
    setStatus(isInCheck(SIDE.RED) ? "红方被将军" : "轮到红方");
    render();
}

function chooseAiMove(moves) {
    let bestScore = -Infinity;
    const bestMoves = [];
    for (const move of moves) {
        const snapshot = cloneBoard(board);
        applyMove(move, false);
        const score = strategicSearch(SIDE.RED, SIDE.BLACK, AI_STRATEGIC_DEPTH - 1, -Infinity, Infinity);
        board = snapshot;
        if (score > bestScore) {
            bestScore = score;
            bestMoves.length = 0;
            bestMoves.push(move);
        } else if (score === bestScore) {
            bestMoves.push(move);
        }
    }
    return bestMoves[Math.floor(Math.random() * bestMoves.length)];
}

function strategicSearch(sideToMove, aiSide, depth, alpha, beta) {
    if (!hasGeneral(aiSide)) {
        return -AI_WIN_SCORE - depth;
    }
    if (!hasGeneral(opponent(aiSide))) {
        return AI_WIN_SCORE + depth;
    }

    const legal = legalMoves(sideToMove);
    if (legal.length === 0) {
        if (isInCheck(sideToMove)) {
            return sideToMove === aiSide ? -AI_MATE_SCORE - depth : AI_MATE_SCORE + depth;
        }
        return evaluatePosition(aiSide);
    }

    if (depth <= 0) {
        return tacticalSearch(sideToMove, aiSide, AI_TACTICAL_DEPTH, alpha, beta);
    }

    const moves = orderedMoves(sideToMove, aiSide, AI_STRATEGIC_MOVE_LIMIT, false);
    let bestScore = sideToMove === aiSide ? -Infinity : Infinity;
    for (const move of moves) {
        const snapshot = cloneBoard(board);
        applyMove(move, false);
        const score = strategicSearch(opponent(sideToMove), aiSide, depth - 1, alpha, beta);
        board = snapshot;

        if (sideToMove === aiSide) {
            bestScore = Math.max(bestScore, score);
            alpha = Math.max(alpha, bestScore);
        } else {
            bestScore = Math.min(bestScore, score);
            beta = Math.min(beta, bestScore);
        }

        if (beta <= alpha) {
            break;
        }
    }
    return bestScore;
}

function tacticalSearch(sideToMove, aiSide, depth, alpha, beta) {
    if (depth <= 0 || !hasGeneral(SIDE.RED) || !hasGeneral(SIDE.BLACK)) {
        return evaluatePosition(aiSide);
    }

    const moves = orderedMoves(sideToMove, aiSide, AI_TACTICAL_MOVE_LIMIT, true);
    if (moves.length === 0) {
        return evaluatePosition(aiSide);
    }

    let bestScore = sideToMove === aiSide ? -Infinity : Infinity;
    for (const move of moves) {
        const snapshot = cloneBoard(board);
        applyMove(move, false);
        const score = tacticalSearch(opponent(sideToMove), aiSide, depth - 1, alpha, beta);
        board = snapshot;

        if (sideToMove === aiSide) {
            bestScore = Math.max(bestScore, score);
            alpha = Math.max(alpha, bestScore);
        } else {
            bestScore = Math.min(bestScore, score);
            beta = Math.min(beta, bestScore);
        }

        if (beta <= alpha) {
            break;
        }
    }
    return bestScore;
}

function evaluatePosition(side) {
    if (!hasGeneral(side)) {
        return -AI_WIN_SCORE;
    }
    if (!hasGeneral(opponent(side))) {
        return AI_WIN_SCORE;
    }

    let score = 0;
    for (let row = 0; row < ROWS; row += 1) {
        for (let col = 0; col < COLS; col += 1) {
            const piece = board[row][col];
            if (piece.side === side) {
                score += (pieceValue[piece.type] || 0) + positionalValue(piece, { row, col }, side);
            } else if (piece.side === opponent(side)) {
                score -= (pieceValue[piece.type] || 0) + positionalValue(piece, { row, col }, opponent(side));
            }
        }
    }

    score += threatScore(side) * 2;
    score -= threatScore(opponent(side)) * 3;
    if (isInCheck(opponent(side))) {
        score += 260;
    }
    if (isInCheck(side)) {
        score -= 360;
    }
    return score;
}

function positionalValue(piece, pos, side) {
    const forward = side === SIDE.RED ? 9 - pos.row : pos.row;
    const center = 4 - Math.abs(pos.col - 4);
    switch (piece.type) {
        case TYPE.GENERAL:
            return 10 - Math.abs(pos.col - 4) * 4;
        case TYPE.ADVISOR:
        case TYPE.ELEPHANT:
            return 8;
        case TYPE.HORSE:
            return center * 8 + forward * 3;
        case TYPE.CHARIOT:
            return center * 5 + forward * 2;
        case TYPE.CANNON:
            return center * 7 + forward * 2;
        case TYPE.SOLDIER:
            return forward * 10 + (forward >= 5 ? center * 6 + 25 : 0);
        default:
            return 0;
    }
}

function threatScore(side) {
    const threats = [];
    for (const move of legalMoves(side)) {
        let score = 0;
        const attacker = board[move.from.row][move.from.col];
        const target = board[move.to.row][move.to.col];
        if (target.type !== TYPE.EMPTY && target.side === opponent(side)) {
            score += (pieceValue[target.type] || 0) - Math.floor((pieceValue[attacker.type] || 0) / 8);
        }
        if (givesCheck(move, side)) {
            score += 180;
        }
        if (score > 0) {
            threats.push(score);
        }
    }

    threats.sort((left, right) => right - left);
    return threats.slice(0, 3).reduce((sum, value, index) => sum + Math.floor(value / (index + 1)), 0);
}

function orderedMoves(side, aiSide, limit, tacticalOnly) {
    let moves = legalMoves(side);
    if (tacticalOnly) {
        const mustAnswerCheck = isInCheck(side);
        moves = moves.filter((move) => mustAnswerCheck || isCapture(move, side));
    }

    const scoredMoves = moves.map((move) => ({
        move,
        score: moveOrderScore(move, side, aiSide),
    }));
    scoredMoves.sort((left, right) => right.score - left.score);
    const limitedMoves = limit > 0 ? scoredMoves.slice(0, limit) : scoredMoves;
    return limitedMoves.map((entry) => entry.move);
}

function moveOrderScore(move, side, aiSide) {
    const attacker = board[move.from.row][move.from.col];
    const target = board[move.to.row][move.to.col];
    let score = 0;
    if (target.type !== TYPE.EMPTY && target.side === opponent(side)) {
        score += (pieceValue[target.type] || 0) * 12 - (pieceValue[attacker.type] || 0);
    }
    if (givesCheck(move, side)) {
        score += 2200;
    }

    const snapshot = cloneBoard(board);
    applyMove(move, false);
    score += Math.floor(evaluateMaterial(aiSide) / 10);
    if (isInCheck(side)) {
        score -= 500;
    }
    board = snapshot;
    return score;
}

function evaluateMaterial(side) {
    let score = 0;
    for (let row = 0; row < ROWS; row += 1) {
        for (let col = 0; col < COLS; col += 1) {
            const piece = board[row][col];
            if (piece.side === side) {
                score += pieceValue[piece.type] || 0;
            } else if (piece.side === opponent(side)) {
                score -= pieceValue[piece.type] || 0;
            }
        }
    }
    return score;
}

function isCapture(move, side) {
    const target = board[move.to.row][move.to.col];
    return target.type !== TYPE.EMPTY && target.side === opponent(side);
}

function givesCheck(move, side) {
    const snapshot = cloneBoard(board);
    applyMove(move, false);
    const checking = isInCheck(opponent(side));
    board = snapshot;
    return checking;
}

function checkGameEndAfterMove(nextSide, winMessage) {
    if (!hasGeneral(nextSide)) {
        endGame(winMessage);
        return true;
    }

    const moves = legalMoves(nextSide);
    if (moves.length === 0) {
        endGame(winMessage);
        return true;
    }
    return false;
}

function endGame(message) {
    gameOver = true;
    turn = SIDE.NONE;
    selected = null;
    legalForSelected = [];
    setStatus(message);
    showResultOverlay(message);
}

function showResultOverlay(message) {
    resultTitle.textContent = message;
    buildConfetti();
    resultOverlay.hidden = false;
    resultOverlay.classList.remove("celebrate");
    void resultOverlay.offsetWidth;
    resultOverlay.classList.add("celebrate");
}

function hideResultOverlay() {
    resultOverlay.hidden = true;
    resultOverlay.classList.remove("celebrate");
}

function setStatus(text) {
    statusText.textContent = text;
}

function addMoveLog(sideLabel, move, captured) {
    const item = document.createElement("li");
    const captureText = captured.type === TYPE.EMPTY ? "" : `，吃${pieceName(captured)}`;
    item.textContent = `${sideLabel}: ${formatPos(move.from)} -> ${formatPos(move.to)}${captureText}`;
    moveList.prepend(item);
}

function formatPos(pos) {
    return `${pos.row},${pos.col}`;
}

function pieceName(piece) {
    if (piece.type === TYPE.EMPTY) {
        return "";
    }
    return piece.side === SIDE.RED ? redNames[piece.type] : blackNames[piece.type];
}

function cloneBoard(source) {
    return source.map((row) => row.map((piece) => ({ ...piece })));
}

function applyMove(move, recordLastMove = true, side = SIDE.NONE) {
    board[move.to.row][move.to.col] = board[move.from.row][move.from.col];
    board[move.from.row][move.from.col] = emptyPiece();
    if (recordLastMove) {
        lastMove = { from: { ...move.from }, to: { ...move.to }, side };
    }
}

function legalMoves(side) {
    const result = [];
    for (let row = 0; row < ROWS; row += 1) {
        for (let col = 0; col < COLS; col += 1) {
            const from = { row, col };
            if (board[row][col].side !== side) {
                continue;
            }

            for (const move of pseudoMoves(from)) {
                const snapshot = cloneBoard(board);
                applyMove(move, false);
                const safe = !isInCheck(side);
                board = snapshot;
                if (safe) {
                    result.push(move);
                }
            }
        }
    }
    return result;
}

function isInCheck(side) {
    const general = findGeneral(side);
    if (!inBounds(general)) {
        return true;
    }

    const enemy = opponent(side);
    for (let row = 0; row < ROWS; row += 1) {
        for (let col = 0; col < COLS; col += 1) {
            const from = { row, col };
            if (board[row][col].side !== enemy) {
                continue;
            }

            for (const move of pseudoMoves(from)) {
                if (samePos(move.to, general)) {
                    return true;
                }
            }
        }
    }
    return false;
}

function hasGeneral(side) {
    return inBounds(findGeneral(side));
}

function findGeneral(side) {
    for (let row = 0; row < ROWS; row += 1) {
        for (let col = 0; col < COLS; col += 1) {
            const piece = board[row][col];
            if (piece.side === side && piece.type === TYPE.GENERAL) {
                return { row, col };
            }
        }
    }
    return { row: -1, col: -1 };
}

function pseudoMoves(from) {
    const piece = board[from.row][from.col];
    if (piece.type === TYPE.EMPTY) {
        return [];
    }

    switch (piece.type) {
        case TYPE.GENERAL:
            return generalMoves(from, piece.side);
        case TYPE.ADVISOR:
            return advisorMoves(from, piece.side);
        case TYPE.ELEPHANT:
            return elephantMoves(from, piece.side);
        case TYPE.HORSE:
            return horseMoves(from, piece.side);
        case TYPE.CHARIOT:
            return slidingMoves(from, piece.side, false);
        case TYPE.CANNON:
            return slidingMoves(from, piece.side, true);
        case TYPE.SOLDIER:
            return soldierMoves(from, piece.side);
        default:
            return [];
    }
}

function generalMoves(from, side) {
    const moves = [];
    for (const [dr, dc] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
        const to = { row: from.row + dr, col: from.col + dc };
        if (inPalace(to, side) && canLand(to, side)) {
            moves.push({ from, to });
        }
    }

    const enemyGeneral = findGeneral(opponent(side));
    if (enemyGeneral.col === from.col && clearLine(from, enemyGeneral)) {
        moves.push({ from, to: enemyGeneral });
    }
    return moves;
}

function advisorMoves(from, side) {
    const moves = [];
    for (const [dr, dc] of [[1, 1], [1, -1], [-1, 1], [-1, -1]]) {
        const to = { row: from.row + dr, col: from.col + dc };
        if (inPalace(to, side) && canLand(to, side)) {
            moves.push({ from, to });
        }
    }
    return moves;
}

function elephantMoves(from, side) {
    const moves = [];
    for (const [dr, dc] of [[2, 2], [2, -2], [-2, 2], [-2, -2]]) {
        const eye = { row: from.row + dr / 2, col: from.col + dc / 2 };
        const to = { row: from.row + dr, col: from.col + dc };
        const ownSide = side === SIDE.RED ? to.row >= 5 : to.row <= 4;
        if (inBounds(to) && ownSide && board[eye.row][eye.col].type === TYPE.EMPTY && canLand(to, side)) {
            moves.push({ from, to });
        }
    }
    return moves;
}

function horseMoves(from, side) {
    const moves = [];
    const jumps = [
        [-2, -1, -1, 0], [-2, 1, -1, 0], [2, -1, 1, 0], [2, 1, 1, 0],
        [-1, -2, 0, -1], [1, -2, 0, -1], [-1, 2, 0, 1], [1, 2, 0, 1],
    ];
    for (const [dr, dc, lr, lc] of jumps) {
        const leg = { row: from.row + lr, col: from.col + lc };
        const to = { row: from.row + dr, col: from.col + dc };
        if (inBounds(to) && board[leg.row][leg.col].type === TYPE.EMPTY && canLand(to, side)) {
            moves.push({ from, to });
        }
    }
    return moves;
}

function slidingMoves(from, side, cannon) {
    const moves = [];
    for (const [dr, dc] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
        let jumped = false;
        let to = { row: from.row + dr, col: from.col + dc };
        while (inBounds(to)) {
            const target = board[to.row][to.col];
            if (!cannon) {
                if (target.type === TYPE.EMPTY) {
                    moves.push({ from, to: { ...to } });
                } else {
                    if (target.side !== side) {
                        moves.push({ from, to: { ...to } });
                    }
                    break;
                }
            } else if (!jumped) {
                if (target.type === TYPE.EMPTY) {
                    moves.push({ from, to: { ...to } });
                } else {
                    jumped = true;
                }
            } else if (target.type !== TYPE.EMPTY) {
                if (target.side !== side) {
                    moves.push({ from, to: { ...to } });
                }
                break;
            }
            to = { row: to.row + dr, col: to.col + dc };
        }
    }
    return moves;
}

function soldierMoves(from, side) {
    const moves = [];
    const forward = side === SIDE.RED ? -1 : 1;
    addLanding(moves, from, { row: from.row + forward, col: from.col }, side);
    if (crossedRiver(from, side)) {
        addLanding(moves, from, { row: from.row, col: from.col - 1 }, side);
        addLanding(moves, from, { row: from.row, col: from.col + 1 }, side);
    }
    return moves;
}

function addLanding(moves, from, to, side) {
    if (canLand(to, side)) {
        moves.push({ from, to });
    }
}

function canLand(to, side) {
    return inBounds(to) && board[to.row][to.col].side !== side;
}

function inBounds(pos) {
    return pos.row >= 0 && pos.row < ROWS && pos.col >= 0 && pos.col < COLS;
}

function inPalace(pos, side) {
    if (pos.col < 3 || pos.col > 5) {
        return false;
    }
    return side === SIDE.RED ? pos.row >= 7 && pos.row <= 9 : pos.row >= 0 && pos.row <= 2;
}

function crossedRiver(pos, side) {
    return side === SIDE.RED ? pos.row <= 4 : pos.row >= 5;
}

function clearLine(from, to) {
    const dr = Math.sign(to.row - from.row);
    const dc = Math.sign(to.col - from.col);
    let row = from.row + dr;
    let col = from.col + dc;
    while (row !== to.row || col !== to.col) {
        if (board[row][col].type !== TYPE.EMPTY) {
            return false;
        }
        row += dr;
        col += dc;
    }
    return true;
}

function samePos(a, b) {
    return a && b && a.row === b.row && a.col === b.col;
}

function opponent(side) {
    return side === SIDE.RED ? SIDE.BLACK : SIDE.RED;
}

resetButton.addEventListener("click", resetBoard);
overlayResetButton.addEventListener("click", resetBoard);
resetBoard();
