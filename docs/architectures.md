```mermaid
flowchart TD
    SQL["SQL / CLI"] --> Parser["Parser"]
    Parser --> Binder["Binder + Catalog"]
    Binder --> Planner["Planner + Optimizer"]
    Planner --> Executor["Executor"]

    Executor --> Table["TableHeap"]
    Executor --> Index["Index"]
    Table --> Buffer["BufferPool"]
    Index --> Buffer
    Buffer --> Disk["DiskManager"]
    Disk --> Files["Database Files"]

    Transaction["Transaction Manager"] -.-> Executor
    Transaction -.-> Table
    Transaction -.-> Index
    Recovery["WAL + Recovery"] -.-> Buffer
    Recovery -.-> Disk

    classDef main fill:#D9E6FF,stroke:#5570FF,stroke-width:1.5px,color:#111827
    classDef future fill:#EEF3FF,stroke:#5570FF,stroke-width:1.5px,stroke-dasharray:5 3,color:#111827

    class SQL,Parser,Binder,Planner,Executor,Table,Index,Buffer,Disk,Files main
    class Transaction,Recovery future

    linkStyle default stroke:#737B8C,stroke-width:1.3px
```

![alt text](miniTubArchitectures.png)



flowchart TD
  CLI[SQL / CLI Shell] --> P[Parser<br/>Lexer + AST]
  P --> B[Binder<br/>resolve names & types]
  B --> PL[Planner<br/>AST to plan tree]
  PL --> O[Optimizer<br/>rule-based rewrites]
  O --> E[Execution Engine<br/>Volcano Init / Next]

  E --> TH[TableHeap<br/>slotted pages, tuples, RID]
  E --> IX[Index<br/>B+ Tree / Extendible Hash]
  TH --> BP[Buffer Pool Manager<br/>LRU-K replacer, page guards]
  IX --> BP
  BP --> DS[Disk Scheduler<br/>I/O queue + worker thread]
  DS --> DM[Disk Manager<br/>read / write 4KB pages]
  DM --> DB[(minitub.db)]

  CAT[Catalog<br/>tables, indexes, schemas]
  B -.-> CAT
  O -.-> CAT
  E -.-> CAT

  TM[Transaction Manager<br/>MVCC, undo logs, watermark GC]
  E -.-> TM
  TM -.-> TH

  WAL[Log Manager<br/>WAL, group commit]:::future
  REC[Recovery Manager<br/>ARIES: analysis, redo, undo]:::future
  BP -.->|flush log before page| WAL
  TM -.->|commit waits for fsync| WAL
  WAL --> LOG[(minitub.log)]
  LOG --> REC

  classDef future stroke-dasharray:5 3


  
