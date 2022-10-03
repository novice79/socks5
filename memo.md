<!-- delete all releases and tags -->
gh release list \
| grep -oE "v[0-9]{8}.[0-9]{6}" \
| uniq | \
while read -r line; do 
gh release delete -y "$line"; 
done

git ls-remote --tags origin | awk '{print $2}' | xargs -n 1 git push --delete origin


