import numpy as np
import polars as pl
import sys

np.random.seed(67)
rng = np.random.default_rng()

N_STRINGS=250_000
MIN_LEN=10
MAX_LEN=60

def gen_used_strings():
    alphabet = list("abcdefghijklmnopqrstuvwxyz0123456789")
    char_arrs = rng.choice(alphabet, size=(N_STRINGS, MAX_LEN))
    full_strings = char_arrs.view(f"U{MAX_LEN}").ravel()

    lengths = rng.integers(MIN_LEN, MAX_LEN + 1, size=N_STRINGS)

    used_strings = pl.DataFrame({
        "str": full_strings,
        "length": lengths
    }).lazy().select(pl.col("str").str.slice(0, pl.col("length")).alias("str"))

    return used_strings

used_strings = gen_used_strings().with_row_index()
find_strings = rng.choice(range(N_STRINGS), size=N_STRINGS*10)
find_strings = used_strings.join(pl.DataFrame({"index": find_strings}).lazy(), how="right", on="index").select("str")

used_strings = used_strings.select(pl.format("+ {}", pl.col("str")).alias("str"))
find_strings = find_strings.select(pl.format("q {}", pl.col("str")).alias("str"))

df = pl.concat((used_strings, find_strings))
df.sink_csv(sys.stdout, include_header=False)
