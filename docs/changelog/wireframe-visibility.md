## Wireframe overlay now visible through pseudocolor

There was an issue when combining overlay wireframes with pseudocolor renderings
where the pseudocolor rendering would obscure the wireframe. Viskores now
detects when a wireframe is rendered as an overlay and ignores the depth buffer
to ensure visibility.
