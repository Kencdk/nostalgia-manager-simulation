import sqlite3
c = sqlite3.connect(r'D:/DEV/Nostalgia/NostalgiaManager/data/Data.db')
tables = c.execute("SELECT name FROM sqlite_master WHERE type='table'").fetchall()
print("Tables:", tables)
for t in tables:
    cols = c.execute(f"PRAGMA table_info(\"{t[0]}\")").fetchall()
    print(f"\n{t[0]} columns:", [col[1] for col in cols])
c.close()
